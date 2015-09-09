#include "analyzer.h"
#include <QCoreApplication>
#include <QFile>

int main(int argc, char *argv[])
{    
	bool ok = argc <= 3;
	if (!ok)
	{
		std::cout << "Wrong arguments. Argument format : output filename (optional), port (optional)";	
		return 1;
	}
		
	//output filename
	QString fileName;
	if (argc >= 2){
		fileName = QString(argv[1]);
	}

	//port
	quint16 port = 7777;	
	if (argc == 3)
	{
		port = QString(argv[2]).toUShort(&ok);

		if (!ok){
			std::cout << QString("Incorrect arg %1\n").arg(argv[1]).toStdString();
			return 1;
		}
	}
	
	//Choosing output and starting
	QCoreApplication a(argc, argv);
	std::unique_ptr<Analyzer> analyzer;
	std::ofstream ofstr;

	if (fileName.isEmpty()){ //cout output
		 analyzer = std::unique_ptr<Analyzer> (new Analyzer);
	}
	else //file
	{				
        ofstr.open(fileName.toLocal8Bit());
		if (!ofstr.is_open()){
			std::cout << QString("Cannot open file : %1").arg(fileName).toStdString();
			return 1;
		}
		analyzer = std::unique_ptr<Analyzer>(new Analyzer(&ofstr));
	}
		
	if (analyzer->startServer(port))
	{			
		std::cout << QString("Listening to port %1\n").arg(port).toStdString();	
		QObject::connect(analyzer.get(), SIGNAL(finished()), &a, SLOT(quit()));
		a.exec();
	}
	else{
		std::cout << QString("Error binding port %1\n").arg(port).toStdString();	
	}

	if (ofstr.is_open()){
		ofstr.close();
	}
}


