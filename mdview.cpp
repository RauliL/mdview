#include <cstdlib>
#include <fstream>
#include <iostream>

#include <gtkmm.h>
#include <webkit2/webkit2.h>

#include "./ext/hoedown/src/html.h"

static const int DEFAULT_WIDTH = 860;
static const int DEFAULT_HEIGHT = 480;

static void set_webkit_settings(WebKitSettings*);
static void on_mouse_target_change(
  WebKitWebView*,
  WebKitHitTestResult*,
  guint,
  void*
);

class MainWindow : public Gtk::Window
{
public:
  explicit MainWindow();

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
    "mouse-target-changed",
    G_CALLBACK(on_mouse_target_change),
    static_cast<void*>(this)
  );

  set_title("MDView");
  set_border_width(0);
  set_default_size(DEFAULT_WIDTH, DEFAULT_HEIGHT);
  add(*m_web_view_widget);
  show_all();
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

static bool parse_file(const std::string& filename, std::string& result)
{
  std::ifstream input(filename);
  std::stringstream buffer;
  hoedown_renderer* renderer;
  hoedown_document* document;
  hoedown_buffer* mdbuffer;

  if (!input.good())
  {
    std::cerr << "Unable to open `"
              << filename
              << "' for reading."
              << std::endl;

    return false;
  }
  buffer << input.rdbuf();
  input.close();
  renderer = hoedown_html_renderer_new(HOEDOWN_HTML_ESCAPE, 0);
  document = hoedown_document_new(
    renderer,
    HOEDOWN_EXT_FENCED_CODE,
    16
  );
  mdbuffer = hoedown_buffer_new(16);
  hoedown_document_render(
    document,
    mdbuffer,
    reinterpret_cast<const uint8_t*>(buffer.str().c_str()),
    buffer.str().length()
  );

  result.assign(
    reinterpret_cast<const char*>(mdbuffer->data),
    mdbuffer->size
  );

  hoedown_buffer_free(mdbuffer);
  hoedown_document_free(document);
  hoedown_html_renderer_free(renderer);

  return true;
}

static void set_webkit_settings(WebKitSettings* settings)
{
  webkit_settings_set_enable_java(settings, false);
  webkit_settings_set_enable_javascript(settings, false);
  webkit_settings_set_enable_plugins(settings, false);
}

static void on_mouse_target_change(WebKitWebView* web_view,
                                   WebKitHitTestResult* hit_test,
                                   guint modifiers,
                                   void*)
{
  auto context = webkit_hit_test_result_get_context(hit_test);

  // Display hovered links in the console.
  // TODO: Modify the web kit view so that clicked links open in system browser
  // instead.
  if ((context & WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK))
  {
    std::cout << webkit_hit_test_result_get_link_uri(hit_test) << std::endl;
  }
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
  else if (!parse_file(argv[1], result))
  {
    return EXIT_FAILURE;
  }
  app->activate();
  static_cast<MainWindow*>(app->get_active_window())->set_html(result);

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
