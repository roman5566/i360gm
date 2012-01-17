#ifndef RAW_H
#define RAW_H

#include "Xbox/Game.h"

class Raw : public Game
{
	Q_OBJECT

public:
	Raw(QString path);
	bool Initialize();

	QVariant getField(int column);
	uint getHash();
	QString getExecutable();
	static bool isRaw(QString path);

	QString getTitleId();
	QString getName();

private:
	QString _titleId, _name;
};

#endif