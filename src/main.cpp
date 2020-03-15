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
#include <cstring>
#include <iostream>

#include "./window.hpp"

static int on_command_line(
  const Glib::RefPtr<Gio::ApplicationCommandLine>&,
  Glib::RefPtr<Gtk::Application>&
);

int main(int argc, char** argv)
{
  auto app = Gtk::Application::create(
    "dev.rauli.mdview",
    Gio::APPLICATION_HANDLES_COMMAND_LINE |
    Gio::APPLICATION_NON_UNIQUE
  );
  MDView::Window window;

  app->signal_command_line().connect(
    sigc::bind(sigc::ptr_fun(&on_command_line), app),
    false
  );

  return app->run(window, argc, argv);
}

static int
on_command_line(
  const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line,
  Glib::RefPtr<Gtk::Application>& app
)
{
  int argc;
  auto argv = command_line->get_arguments(argc);
  Glib::OptionContext context;
  Glib::OptionGroup gtk_group(gtk_get_option_group(true));
  MDView::Window* window;

  context.add_group(gtk_group);

  if (!context.parse(argc, argv))
  {
    std::exit(EXIT_FAILURE);
  }
  else if (argc > 2)
  {
    std::cerr << "Too many arguments given." << std::endl;
    std::exit(EXIT_FAILURE);
  }

  app->activate();
  window = static_cast<MDView::Window*>(app->get_active_window());

  if (argc == 2)
  {
    if (!std::strcmp(argv[1], "-"))
    {
      std::string input;

      for (std::string line; std::getline(std::cin, line);)
      {
        input.append(line);
        input.append(1, '\n');
      }
      window->set_markdown(input);
    }
    else if (!window->show_file(argv[1]))
    {
      std::exit(EXIT_FAILURE);
    }
  }
  else if (!window->open_file_chooser_dialog())
  {
    std::exit(EXIT_SUCCESS);
  }

  return EXIT_SUCCESS;
}
