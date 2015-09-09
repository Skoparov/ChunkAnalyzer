#include "loader.h"
#include <QCoreApplication>

/*
void createTestFile()
{
	QFile file("test");
	file.open(QIODevice::WriteOnly);

	quint64 f1 = 0x11223344AABBCCDD;
	quint64 f2 = 0xAABBCCDD11223344;

	QDataStream out(&file);
	for (int i = 0; i < 5000; ++i){
		out << f1 << f2;
	}

	file.close();
}*/

int main(int argc, char *argv[])
{
	//createTestFile();	

	if (argc < 4)
	{
		std::cout << "Wrong arguments. Argument format : server addr, port, dirs\n";
		return 1;
	}	

	//server addr
	QString addr (argv[1]);

	//port
	quint16 port = 7777;	
	bool ok;
	port = QString(argv[2]).toUShort(&ok);

	if (!ok){
		std::cout << QString("Incorrect port : %1").arg(argv[2]).toStdString();
		return 1;
	}

	//directories
	QStringList dirs;
	for (int dirNum = 2; dirNum < argc; ++dirNum){
		dirs.push_back(argv[dirNum]);		
	}

	//start
    QCoreApplication a(argc, argv);
    Loader l(addr, port);  	

	if (l.manageDirs(dirs))
	{	
		QObject::connect(&l, SIGNAL(finished()), &a, SLOT(quit()));
		return a.exec();
	}
}
