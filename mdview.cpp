/*
 * Copyright (c) 2018, Rauli Laine
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
#include <ctime>
#include <fstream>
#include <iostream>

#include <gtkmm.h>
#include <webkit2/webkit2.h>

#include "./ext/hoedown/src/html.h"

static const int DEFAULT_WIDTH = 860;
static const int DEFAULT_HEIGHT = 480;

static void set_webkit_settings(WebKitSettings*);
static gboolean on_decide_policy(
  WebKitWebView*,
  WebKitPolicyDecision*,
  WebKitPolicyDecisionType,
  void*
);

class MainWindow : public Gtk::Window
{
public:
  explicit MainWindow();

  void show_file(const std::string& path);
  void set_markdown(const std::string& markdown);
  void set_html(const std::string& html);

protected:
  void on_show();
  bool on_key_press_event(GdkEventKey* event);

private:
  WebKitWebView* m_web_view;
  Gtk::Widget* m_web_view_widget;
};

MainWindow::MainWindow()
  : m_web_view(WEBKIT_WEB_VIEW(webkit_web_view_new()))
  , m_web_view_widget(Glib::wrap(GTK_WIDGET(m_web_view)))
{
  set_webkit_settings(webkit_web_view_get_settings(m_web_view));

  g_signal_connect(
    G_OBJECT(m_web_view),
    "decide-policy",
    G_CALLBACK(on_decide_policy),
    static_cast<void*>(gobj())
  );

  set_title("mdview");
  set_border_width(0);
  set_default_size(DEFAULT_WIDTH, DEFAULT_HEIGHT);
  add(*m_web_view_widget);
  maximize();
  show_all();
}

void MainWindow::show_file(const std::string& path)
{
  std::ifstream input(path);
  std::stringstream buffer;

  if (!input.good())
  {
    std::cerr << "Unable to open `" << path << "' for reading." << std::endl;
    std::exit(EXIT_FAILURE);
    return;
  }
  buffer << input.rdbuf();
  input.close();
  set_markdown(buffer.str());
  set_title(path + " - mdview");
}

void MainWindow::set_markdown(const std::string& markdown)
{
  auto renderer = hoedown_html_renderer_new(HOEDOWN_HTML_ESCAPE, 0);
  auto document = hoedown_document_new(
    renderer,
    HOEDOWN_EXT_FENCED_CODE,
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

void MainWindow::set_html(const std::string& html)
{
  webkit_web_view_load_html(m_web_view, html.c_str(), nullptr);
}

void MainWindow::on_show()
{
  Gtk::Window::on_show();
  m_web_view_widget->grab_focus();
}

bool MainWindow::on_key_press_event(GdkEventKey* event)
{
  // Terminate the application when user presses ^Q anywhere inside the main
  // window.
  if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_q)
  {
    std::exit(EXIT_SUCCESS);
  }

  return Gtk::Window::on_key_press_event(event);
}

static void set_webkit_settings(WebKitSettings* settings)
{
  webkit_settings_set_enable_java(settings, false);
  webkit_settings_set_enable_javascript(settings, false);
  webkit_settings_set_enable_plugins(settings, false);
}

static gboolean on_decide_policy(WebKitWebView* web_view,
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
        const auto uri = webkit_uri_request_get_uri(
          webkit_navigation_action_get_request(
            webkit_navigation_policy_decision_get_navigation_action(
              navigation_decision
            )
          )
        );

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

static int on_command_line(
  const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line,
  Glib::RefPtr<Gtk::Application>& app
)
{
  int argc = 0;
  auto argv = command_line->get_arguments(argc);
  std::string result;

  if (argc != 2)
  {
    std::cerr << "No filename given." << std::endl;

    return EXIT_FAILURE;
  }
  app->activate();
  static_cast<MainWindow*>(app->get_active_window())->show_file(argv[1]);

  return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
  auto app = Gtk::Application::create(
    argc,
    argv,
    "raulil.github.io.mdview",
    Gio::APPLICATION_HANDLES_COMMAND_LINE |
    Gio::APPLICATION_NON_UNIQUE
  );
  MainWindow window;

  app->signal_command_line().connect(
    sigc::bind(sigc::ptr_fun(&on_command_line), app),
    false
  );

  return app->run(window);
}
