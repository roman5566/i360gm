#include "uiMain.h"
#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setStyle(new QCleanlooksStyle);
	Main w;
	w.show();
	return a.exec();
}
