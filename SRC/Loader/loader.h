#ifndef LOADER_H
#define LOADER_H

/**
* @file loader.h
* @brief Contains classes for parsing dirs and sending files to Analyzer
*/

#include <QTcpSocket>
#include <QHash>
#include <QRunnable>
#include <QFile>
#include <QDir>
#include <QThreadPool>
#include <QEventLoop>
#include <QHostAddress>

#include <iostream>
#include <memory>

typedef std::unique_ptr<QTcpSocket> QTcpSocketPtr;
typedef std::unique_ptr<QEventLoop> QEventLoopPtr;
typedef std::unique_ptr<QFile> QFilePtr;
typedef quint64 Chunk;  /**< Elemental data to be collected */	
typedef quint16 BlockSize; /**< Block is comprised of several chunks (at least one) */
 
Q_DECLARE_METATYPE(QAbstractSocket::SocketError);

//////////////////////////////////////////////////
///                 Loader                      ///
//////////////////////////////////////////////////

/**
* Parses dirs and starts a transmission thread for every file
*/

class Loader : public QObject
{
	Q_OBJECT

public:
	Loader(const QString& addr, const quint16& port);
	
public slots:
	bool manageDirs(const QStringList dirs);

private slots:
	void sendFinished();

signals:
	void finished();

private:
	quint16 mPort;
	QString mAddr;
};

//////////////////////////////////////////////////
///                 ChunkSender                ///
//////////////////////////////////////////////////

/**
* Sends it's file to Analyzer block by block
*/

class ChunkSender : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit ChunkSender(const QString filePath, const QString addr, const quint16& port, QObject *parent = 0);  
    void run();

signals:
    void error(QTcpSocket::SocketError socketerror);
    void finished();

private slots:	
	void onError(QAbstractSocket::SocketError error);	
    void sendNextBlock();
    void sendFileData();
	void onServerResponse();
    void write(const QByteArray data);

private:
	QEventLoopPtr mEventLoop;
	QTcpSocketPtr mSocket;

	QString mAddr;
    quint16 mPort;

	BlockSize mPreviousBlockSize;  /**< To check is Analyzer has correctly received the previous data */
    quint16 mChunksInBlock;		  
   
    QString mFilePath;    
	QFilePtr mFile;
};

#endif // LOADER_H
