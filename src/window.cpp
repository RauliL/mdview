/*
 * Copyright (c) 2018-2026, Rauli Laine
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include <maddy/parser.h>

#include "./window.hpp"

extern const std::array<gchar, 1090404> highlight_js;
extern const std::array<gchar, 1145> highlight_light_css;
extern const std::array<gchar, 626> highlight_dark_css;

extern "C" void
mdview_uri_launch_finished(GObject* launcher_obj, GAsyncResult*, gpointer user_data)
{
  (void) user_data;
  g_object_unref(launcher_obj);
}

namespace MDView
{
  static constexpr int DEFAULT_WIDTH = 640;
  static constexpr int DEFAULT_HEIGHT = 480;

  static void populate_user_content(WebKitUserContentManager* manager);

  static gboolean on_decide_policy(
    WebKitWebView*,
    WebKitPolicyDecision*,
    WebKitPolicyDecisionType,
    void*
  );
  static gboolean on_context_menu(
    WebKitWebView*,
    WebKitContextMenu*,
    WebKitHitTestResult*,
    void*
  );

  static void
  noop_javascript_finished(GObject*, GAsyncResult*, gpointer)
  {
  }

  Window::Window()
    : m_box(Gtk::Orientation::VERTICAL)
    , m_web_view(WEBKIT_WEB_VIEW(webkit_web_view_new()))
    , m_web_view_widget(Glib::wrap(GTK_WIDGET(m_web_view)))
  {
    populate_user_content(
      webkit_web_view_get_user_content_manager(m_web_view)
    );

    set_title("MDView");

    m_web_view_widget->set_hexpand(true);
    m_web_view_widget->set_vexpand(true);
    m_box.append(*m_web_view_widget);
    m_box.append(m_search_bar);

    m_search_bar.set_child(m_search_entry);
    m_search_bar.connect_entry(m_search_entry);
    m_search_bar.set_show_close_button(true);
    m_search_bar.set_key_capture_widget(*m_web_view_widget);

    set_child(m_box);
    set_default_size(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    maximize();

    g_signal_connect(
      G_OBJECT(m_web_view),
      "decide-policy",
      G_CALLBACK(on_decide_policy),
      static_cast<void*>(gobj())
    );
    g_signal_connect(
      G_OBJECT(m_web_view),
      "context-menu",
      G_CALLBACK(on_context_menu),
      nullptr
    );

    m_win_key_controller = Gtk::EventControllerKey::create();
    m_win_key_controller->set_propagation_phase(
      Gtk::PropagationPhase::CAPTURE
    );
    m_win_key_controller->signal_key_pressed().connect(
      sigc::mem_fun(*this, &Window::on_shortcut_keys),
      false
    );
    add_controller(m_win_key_controller);

    m_web_key_controller = Gtk::EventControllerKey::create();
    m_web_key_controller->signal_key_pressed().connect(
      sigc::mem_fun(*this, &Window::on_web_keyboard),
      false
    );
    m_web_view_widget->add_controller(m_web_key_controller);

    m_search_bar.property_search_mode_enabled().signal_changed().connect(
      sigc::mem_fun(*this, &Window::on_search_mode_enabled_change)
    );
    m_search_entry.signal_search_changed().connect(
      sigc::mem_fun(*this, &Window::on_search_text_changed)
    );

    signal_map().connect(
      sigc::mem_fun(*this, &Window::on_focus_web_view_when_mapped)
    );
  }

  bool
  Window::open_file_chooser_dialog()
  {
    auto file_dialog = Gtk::FileDialog::create();
    auto filter_markdown = Gtk::FileFilter::create();
    auto filter_any = Gtk::FileFilter::create();
    auto filters = Gio::ListStore<Gtk::FileFilter>::create();
    bool completed = false;
    bool accepted = false;
    auto ctx = Glib::MainContext::get_default();

    file_dialog->set_title("Open Markdown file");

    filter_markdown->set_name("Markdown files");
    filter_markdown->add_mime_type("text/markdown");
    filter_markdown->add_pattern("*.md");
    filter_markdown->add_pattern("*.markdown");

    filter_any->set_name("Any files");
    filter_any->add_pattern("*");

    filters->append(filter_markdown);
    filters->append(filter_any);
    file_dialog->set_filters(filters);
    file_dialog->set_default_filter(filter_markdown);

    file_dialog->open(
      *this,
      [&, file_dialog](const Glib::RefPtr<Gio::AsyncResult>& result) {
        try
        {
          auto file = file_dialog->open_finish(result);
          const auto path = file->get_path();

          accepted = path.empty()
            ? false
            : show_file(path);
        }
        catch (const Gtk::DialogError& err)
        {
          if (
            err.code() != Gtk::DialogError::Code::DISMISSED &&
            err.code() != Gtk::DialogError::Code::CANCELLED
          )
          {
            std::cerr << "File dialog: " << err.what() << std::endl;
          }
          accepted = false;
        }

        completed = true;
      }
    );

    while (!completed)
    {
      ctx->iteration(true);
    }

    return accepted;
  }

  bool
  Window::show_file(const std::string& path)
  {
    std::ifstream input(path);
    std::stringstream buffer;

    if (!input.good())
    {
      std::cerr << "Unable to open `" << path << "' for reading." << std::endl;

      return false;
    }
    buffer << input.rdbuf();
    input.close();
    set_markdown(buffer.str());
    set_title(path + " - mdview");

    return true;
  }

  void
  Window::set_markdown(const std::string& markdown)
  {
    std::stringstream input(markdown);
    const auto config = std::make_shared<maddy::ParserConfig>();

    config->enabledParsers = maddy::types::ALL;
    set_html(maddy::Parser(config).Parse(input));
  }

  void
  Window::set_html(const std::string& html)
  {
    webkit_web_view_load_html(m_web_view, html.c_str(), nullptr);
  }

  void
  Window::run_javascript(const std::string& script)
  {
    webkit_web_view_evaluate_javascript(
      m_web_view,
      script.c_str(),
      static_cast<gssize>(script.length()),
      nullptr,
      nullptr,
      nullptr,
      noop_javascript_finished,
      nullptr
    );
  }

  void
  Window::toggle_search_mode()
  {
    m_search_bar.set_search_mode(!m_search_bar.get_search_mode());
  }

  void
  Window::search(const Glib::ustring& text)
  {
    auto find_controller = webkit_web_view_get_find_controller(m_web_view);

    if (text.empty())
    {
      webkit_find_controller_search_finish(find_controller);
    }
    else
    {
      webkit_find_controller_search(
        find_controller,
        text.c_str(),
        WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE,
        25
      );
    }
  }

  void
  Window::search_next()
  {
    webkit_find_controller_search_next(
      webkit_web_view_get_find_controller(m_web_view)
    );
  }

  void
  Window::search_previous()
  {
    webkit_find_controller_search_previous(
      webkit_web_view_get_find_controller(m_web_view)
    );
  }

  void
  Window::on_focus_web_view_when_mapped()
  {
    m_web_view_widget->grab_focus();
  }

  bool
  Window::on_shortcut_keys(
    guint keyval,
    guint,
    Gdk::ModifierType state
  )
  {
    if (
      (state & Gdk::ModifierType::CONTROL_MASK) !=
      Gdk::ModifierType::CONTROL_MASK
    )
    {
      return false;
    }

    if (keyval == GDK_KEY_q)
    {
      std::exit(EXIT_SUCCESS);
    }

    if (keyval == GDK_KEY_f)
    {
      toggle_search_mode();
      return true;
    }

    if (keyval == GDK_KEY_o)
    {
      open_file_chooser_dialog();
      return true;
    }

    return false;
  }

  void
  Window::on_search_mode_enabled_change()
  {
    if (!m_search_bar.get_search_mode())
    {
      m_web_view_widget->grab_focus();
    }
  }

  void
  Window::on_search_text_changed()
  {
    search(m_search_entry.get_text());
  }

  bool
  Window::on_web_keyboard(
    guint keyval,
    guint,
    Gdk::ModifierType state
  )
  {
    if ((state & Gdk::ModifierType::CONTROL_MASK) !=
        Gdk::ModifierType::CONTROL_MASK)
    {
      switch (gdk_keyval_to_unicode(keyval))
      {
      case 'h':
        run_javascript("window.scrollBy({ left: -100 })");
        break;

      case 'j':
        run_javascript("window.scrollBy({ top: 100 })");
        break;

      case 'k':
        run_javascript("window.scrollBy({ top: -100 })");
        break;

      case 'l':
        run_javascript("window.scrollBy({ left: 100 })");
        break;

      case 'n':
        search_next();
        break;

      case 'N':
        search_previous();
        break;

      case '/':
        toggle_search_mode();
        break;

      default:
        return false;
      }

      return true;
    }

    return false;
  }

  static inline bool
  prefers_dark_mode()
  {
    const auto settings = Gio::Settings::create("org.gnome.desktop.interface");
    const auto color_scheme = settings->get_string("color-scheme");

    return (
      color_scheme == "prefer-dark" ||
      color_scheme == "force-dark" ||
      Gtk::Settings::get_default()->property_gtk_application_prefer_dark_theme()
        .get_value()
    );
  }

  static void
  populate_user_content(WebKitUserContentManager* manager)
  {
    if (prefers_dark_mode())
    {
      webkit_user_content_manager_add_style_sheet(
        manager,
        webkit_user_style_sheet_new(
          ":root { color-scheme: light dark; }",
          WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
          WEBKIT_USER_STYLE_LEVEL_USER,
          nullptr,
          nullptr
        )
      );
      webkit_user_content_manager_add_style_sheet(
        manager,
        webkit_user_style_sheet_new(
          highlight_dark_css.data(),
          WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
          WEBKIT_USER_STYLE_LEVEL_USER,
          nullptr,
          nullptr
        )
      );
    } else {
      webkit_user_content_manager_add_style_sheet(
        manager,
        webkit_user_style_sheet_new(
          highlight_light_css.data(),
          WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
          WEBKIT_USER_STYLE_LEVEL_USER,
          nullptr,
          nullptr
        )
      );
    }
    webkit_user_content_manager_add_script(
      manager,
      webkit_user_script_new(
        highlight_js.data(),
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        nullptr,
        nullptr
      )
    );
    webkit_user_content_manager_add_script(
      manager,
      webkit_user_script_new(
        "hljs.highlightAll();",
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_END,
        nullptr,
        nullptr
      )
    );
  }

  static gboolean
  on_decide_policy(
    WebKitWebView*,
    WebKitPolicyDecision* decision,
    WebKitPolicyDecisionType decision_type,
    void* data
  )
  {
    switch (decision_type)
    {
    case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION:
    case WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION:
      {
        auto navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
        auto action =
          webkit_navigation_policy_decision_get_navigation_action(navigation_decision);
        auto navigation_type = webkit_navigation_action_get_navigation_type(action);
        const auto uri = webkit_uri_request_get_uri(
          webkit_navigation_action_get_request(action)
        );

        if (navigation_type == WEBKIT_NAVIGATION_TYPE_OTHER)
        {
          break;
        }

        if (uri)
        {
          GtkUriLauncher* launcher = gtk_uri_launcher_new(uri);
          gtk_uri_launcher_launch(
            launcher,
            static_cast<GtkWindow*>(data),
            nullptr,
            mdview_uri_launch_finished,
            nullptr
          );
        }
        webkit_policy_decision_ignore(decision);
        break;
      }

    default:
      return false;
    }

    return true;
  }

  static gboolean
  on_context_menu(
    WebKitWebView*,
    WebKitContextMenu*,
    WebKitHitTestResult*,
    void*)
  {
    return true;
  }
}
