/*
 * Copyright (c) 2018-2020, Rauli Laine
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
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "../ext/hoedown/src/html.h"

#include "./window.hpp"

namespace MDView
{
  static constexpr int DEFAULT_WIDTH = 640;
  static constexpr int DEFAULT_HEIGHT = 480;

  static void set_webkit_settings(WebKitSettings*);
  static gboolean on_decide_policy(
    WebKitWebView*,
    WebKitPolicyDecision*,
    WebKitPolicyDecisionType,
    void*
  );
  static gboolean on_context_menu(
    WebKitWebView*,
    WebKitContextMenu*,
    GdkEvent*,
    WebKitHitTestResult*,
    void*
  );
  static gboolean on_web_view_key_press(
    WebKitWebView*,
    GdkEventKey*,
    Window*
  );

  Window::Window()
    : m_box(Gtk::ORIENTATION_VERTICAL)
    , m_web_view(WEBKIT_WEB_VIEW(webkit_web_view_new()))
    , m_web_view_widget(Glib::wrap(GTK_WIDGET(m_web_view)))
  {
    set_title("MDView");

    m_box.pack_start(*m_web_view_widget, Gtk::PACK_EXPAND_WIDGET);
    m_box.pack_start(m_search_bar, Gtk::PACK_SHRINK);

    m_search_bar.add(m_search_entry);
    m_search_bar.connect_entry(m_search_entry);
    m_search_bar.set_show_close_button(true);

    add(m_box);
    set_default_size(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    maximize();
    show_all();

    set_webkit_settings(webkit_web_view_get_settings(m_web_view));

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
    g_signal_connect(
      G_OBJECT(m_web_view),
      "key-press-event",
      G_CALLBACK(on_web_view_key_press),
      static_cast<gpointer>(this)
    );

    m_search_bar.property_search_mode_enabled().signal_changed().connect(
      sigc::mem_fun(
        this,
        &Window::on_search_mode_enabled_change
      )
    );
    m_search_entry.signal_search_changed().connect(sigc::mem_fun(
      this,
      &Window::on_search_text_changed
    ));
  }

  bool
  Window::open_file_chooser_dialog()
  {
    auto filter_markdown = Gtk::FileFilter::create();
    auto filter_any = Gtk::FileFilter::create();
    Gtk::FileChooserDialog dialog(
      "Open Markdown file",
      Gtk::FILE_CHOOSER_ACTION_OPEN
    );
    int result;

    dialog.set_transient_for(*this);

    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Open", Gtk::RESPONSE_OK);

    filter_markdown->set_name("Markdown files");
    filter_markdown->add_mime_type("text/markdown");
    filter_markdown->add_pattern("*.md");
    filter_markdown->add_pattern("*.markdown");
    dialog.add_filter(filter_markdown);

    filter_any->set_name("Any files");
    filter_any->add_pattern("*");
    dialog.add_filter(filter_any);

    if ((dialog.run() == Gtk::RESPONSE_OK))
    {
      return show_file(dialog.get_filename());
    }

    return false;
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
    auto renderer = hoedown_html_renderer_new(HOEDOWN_HTML_ESCAPE, 0);
    auto document = hoedown_document_new(
      renderer,
      static_cast<hoedown_extensions>(HOEDOWN_EXT_BLOCK | HOEDOWN_EXT_SPAN),
      16
    );
    auto mdbuffer = hoedown_buffer_new(16);
    std::string result;

    hoedown_document_render(
      document,
      mdbuffer,
      reinterpret_cast<const uint8_t*>(markdown.c_str()),
      markdown.length()
    );
    result.assign(
      reinterpret_cast<const char*>(mdbuffer->data),
      mdbuffer->size
    );

    hoedown_buffer_free(mdbuffer);
    hoedown_document_free(document);
    hoedown_html_renderer_free(renderer);

    set_html(result);
  }

  void
  Window::set_html(const std::string& html)
  {
    webkit_web_view_load_html(m_web_view, html.c_str(), nullptr);
  }

  void
  Window::run_javascript(const std::string& script)
  {
#if (WEBKIT_MAJOR_VERSION >= 2 && WEBKIT_MINOR_VERSION >= 40)
    webkit_web_view_evaluate_javascript(
      m_web_view,
      script.c_str(),
      script.length(),
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr
    );
#else
    webkit_web_view_run_javascript(
      m_web_view,
      script,
      nullptr,
      nullptr,
      nullptr
    );
#endif
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
    } else {
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
  Window::on_show()
  {
    Gtk::Window::on_show();
    m_web_view_widget->grab_focus();
  }

  bool
  Window::on_key_press_event(GdkEventKey* event)
  {
    if ((event->state & GDK_CONTROL_MASK))
    {
      // Terminate the application when user presses ^Q anywhere inside the
      // main window.
      if (event->keyval == GDK_KEY_q)
      {
        std::exit(EXIT_SUCCESS);

        return true;
      }
      // Activate or disable search bar when user presses ^F anywhere inside
      // the main window.
      else if (event->keyval == GDK_KEY_f)
      {
        toggle_search_mode();

        return true;
      }
      // Open file dialog when user presses ^O anywhere inside the main window.
      else if (event->keyval == GDK_KEY_o)
      {
        open_file_chooser_dialog();

        return true;
      }
    }

    return Gtk::Window::on_key_press_event(event);
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

  static void
  set_webkit_settings(WebKitSettings* settings)
  {
#if !(WEBKIT_MAJOR_VERSION >= 2 && WEBKIT_MINOR_VERSION >= 38)
    webkit_settings_set_enable_java(settings, false);
#endif
#if !(WEBKIT_MAJOR_VERSION >= 2 && WEBKIT_MINOR_VERSION >= 32)
    webkit_settings_set_enable_plugins(settings, false);
#endif
  }

  static gboolean
  on_decide_policy(WebKitWebView* web_view,
                   WebKitPolicyDecision* decision,
                   WebKitPolicyDecisionType decision_type,
                   void* data)
  {
    switch (decision_type)
    {
      case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION:
      case WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION:
        {
          auto navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
          auto action = webkit_navigation_policy_decision_get_navigation_action(
            navigation_decision
          );
          auto navigation_type = webkit_navigation_action_get_navigation_type(
            action
          );
          const auto uri = webkit_uri_request_get_uri(
            webkit_navigation_action_get_request(action)
          );

          // Do not open content set directly into the web view in external
          // browser.
          if (navigation_type == WEBKIT_NAVIGATION_TYPE_OTHER)
          {
            break;
          }
          if (uri)
          {
            gtk_show_uri_on_window(
              static_cast<GtkWindow*>(data),
              uri,
              static_cast<guint32>(std::time(nullptr)),
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
  on_context_menu(WebKitWebView*,
                  WebKitContextMenu*,
                  GdkEvent*,
                  WebKitHitTestResult*,
                  void*)
  {
    // Always disable context menu.
    return true;
  }

  static gboolean
  on_web_view_key_press(WebKitWebView*,
                        GdkEventKey* event,
                        Window* window)
  {
    if ((event->state & GDK_CONTROL_MASK))
    {
      return false;
    }
    switch (gdk_keyval_to_unicode(event->keyval))
    {
      case 'h':
        window->run_javascript("window.scrollBy({ left: -100 })");
        break;

      case 'j':
        window->run_javascript("window.scrollBy({ top: 100 })");
        break;

      case 'k':
        window->run_javascript("window.scrollBy({ top: -100 })");
        break;

      case 'l':
        window->run_javascript("window.scrollBy({ left: 100 })");
        break;

      case 'n':
        window->search_next();
        break;

      case 'N':
        window->search_previous();
        break;

      case '/':
        window->toggle_search_mode();
        break;

      default:
        return false;
    }

    return true;
  }
}
