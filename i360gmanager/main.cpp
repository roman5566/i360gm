#include "uiMain.h"
#include <QtGui/QApplication>

Main *mainClass;

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setStyle(new QCleanlooksStyle);
	mainClass = new Main;
	mainClass->show();
	return a.exec();
}
