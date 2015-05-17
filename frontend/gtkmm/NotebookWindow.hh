
#pragma once

#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/label.h>
#include <gtkmm/stock.h>
#include <gtkmm/button.h>
#include <gtkmm/uimanager.h>
#include <gtkmm/cssprovider.h>
#include <glibmm/dispatcher.h>

#include <thread>
#include <mutex>

#include "DocumentThread.hh"
#include "ComputeThread.hh"
#include "GUIBase.hh"
#include "NotebookCanvas.hh"
#include "../common/TeXEngine.hh"

namespace cadabra {

   // Each notebook has one main window which controls it. It has a menu bar, a
   // status pane and one or more panels that represent a view on the document.

   class NotebookWindow : public Gtk::Window, public DocumentThread, public GUIBase {
      public:
         NotebookWindow();
         ~NotebookWindow();
        
         // Virtual functions from GUIBase.

         virtual void add_cell(const DTree&, DTree::iterator, bool) override;
         virtual void remove_cell(const DTree&, DTree::iterator) override;
         virtual void remove_all_cells() override;
         virtual void update_cell(const DTree&, DTree::iterator) override;
			virtual void position_cursor(const DTree&, DTree::iterator) override;

         virtual void on_connect() override;
         virtual void on_disconnect() override;
         virtual void on_network_error() override;

			virtual void process_data() override;

			// TeX stuff
			TeXEngine        engine;

		protected:
			virtual bool on_key_press_event(GdkEventKey*) override;
			virtual bool on_delete_event(GdkEventAny*) override;

			DTree::iterator current_cell;

      private:

			// Main handler which fires whenever the Client object signals 
			// that the document is changing or the network status is modified.
			// Runs on the GUI thread.

			Glib::Dispatcher dispatcher;

			// GUI elements.
			
			Glib::RefPtr<Gtk::ActionGroup> actiongroup;
			Glib::RefPtr<Gtk::UIManager>   uimanager;

			Gtk::VBox                      topbox;
			Gtk::HBox                      supermainbox;
			Gtk::VBox                      mainbox;
//			Gtk::HBox                      buttonbox;
			Gtk::HBox                      statusbarbox;

			// All canvasses which are stored in the ...
			// These pointers are managed by gtkmm.
			std::vector<NotebookCanvas *>  canvasses;
			int                            current_canvas;

         // Buttons
//         Gtk::Button                    b_kill, b_run, b_run_to, b_run_from, b_help, b_stop, b_undo, b_redo;

			// Status bar
			Gtk::ProgressBar               progressbar;
			Gtk::Label                     status_label, kernel_label;

			// GUI data which is the autoritative source for things displayed in
			// the status bars declared above. These strings are filled on the
			// compute thread and then updated into the gui on the gui thread.

			std::mutex                     status_mutex;
			std::string                    status_string, kernel_string;

			// Name and modification data.
			void             update_title();
			void             set_stop_sensitive(bool);
			std::string      name;
			bool             modified;

			// Menu and button callbacks.
			void on_file_new();
			void on_file_open();
			void on_file_save();
			void on_file_save_as();
			void on_file_quit();

			void on_edit_undo();
			void on_edit_cell_is_latex();
			void on_edit_cell_is_python();

			void on_view_split();
			void on_view_close();

			void on_run_runall();
			void on_run_runtocursor();
			void on_run_stop();

			void on_kernel_restart();

			// Todo deque processing logic. This gets called by the dispatcher, but it
			// is also allowed to call this from within NotebookWindow itself. The important
			// thing is that it is run on the GUI thread.
			void process_todo_queue();

			// The following are handlers that get called when the cell
			// gets focus, the content of a cell is changed, the user
			// requests to run it (shift-enter). The last two parameters are
			// always the cell in the DTree and the canvas number.

			bool cell_got_focus(DTree::iterator, int);
			bool cell_content_changed(const std::string&, DTree::iterator, int);
			bool cell_content_execute(DTree::iterator, int);
			
			// Handler for callbacks from TeXView cells.

			bool on_tex_error(const std::string&, DTree::iterator);

			// Styling through CSS
			void                           setup_css_provider();
			Glib::RefPtr<Gtk::CssProvider> css_provider;
	};

};
