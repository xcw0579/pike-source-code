//! A OptionMenu is a widget that allows the user to choose from a
//! list of valid choices. The OptionMenu displays the selected
//! choice. When activated the OptionMenu displays a popup W(Menu)
//! which allows the user to make a new choice.
//! 
//!@code{ GTK.OptionMenu()->set_menu(GTK.Menu()->add( GTK.Menu_item("Option 1") ));@}
//!@xml{<image src='../images/gtk_optionmenu.png'/>@}
//!
//! 
//!
//!

inherit Button;

static OptionMenu create( );
//! Create a new option menu widget
//!
//!

GTK.Menu get_menu( );
//! Returns the W(Menu) associated with the OptionMenu.
//!
//!

OptionMenu remove_menu( );
//! Remove the menu.
//!
//!

OptionMenu set_history( int index );
//! Selects the menu item specified by index making it the newly
//! selected value for the option menu.
//!
//!

OptionMenu set_menu( GTK.Menu menu );
//! Provides the GtkMenu that is popped up to allow the user to choose
//! a new value. You should provide a simple menu avoiding the use of
//! tearoff menu items, submenus, and accelerators.
//!
//!
