#include "uiMain.h"
#include <QtGui/QApplication>

//#include <vld.h>

/** \mainpage
 *
 * \section intro Introduction
 *
 * i360gmanager
 * manages all xbox 360 game backups between different medias.
 *
 * \section formats Formats supported
 *
 * The following file formats are supported:
 * - iso
 * - GOD
 * - Raw extracted iso
 *
 */

Main *mainGui;
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setStyle(new QCleanlooksStyle);
    mainGui = new Main;
	mainGui->show();
	return a.exec();
}
