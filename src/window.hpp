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
#include <string>

#include <gtkmm.h>
#include <webkit2/webkit2.h>

namespace MDView
{
  class Window : public Gtk::Window
  {
  public:
    explicit Window();

    void show_file(const std::string& path);
    void set_markdown(const std::string& markdown);
    void set_html(const std::string& html);
    void run_javascript(const gchar* script);
    void toggle_search_mode();
    void search(const Glib::ustring& text);
    void search_next();
    void search_previous();

  private:
    void on_show();
    bool on_key_press_event(GdkEventKey* event);
    void on_search_mode_enabled_change();
    void on_search_text_changed();

  private:
    Gtk::Box m_box;
    WebKitWebView* m_web_view;
    Gtk::Widget* m_web_view_widget;
    Gtk::SearchBar m_search_bar;
    Gtk::SearchEntry m_search_entry;
  };
}
