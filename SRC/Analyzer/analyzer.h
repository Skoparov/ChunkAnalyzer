#ifndef ANALYZER_H
#define ANALYZER_H

/**
* @file analyzer.h
* @brief Contains classes for receiving and collecting chunks
*/

#include <QTcpSocket>
#include <QTcpServer>
#include <QHash>
#include <QStringList>
#include <QEventLoop>
#include <QRunnable>
#include <QThreadPool>

#include <memory>
#include <iostream>
#include <fstream>

class ChunkCollector;

typedef quint64 Chunk; /**< Elemental data to be collected */	
typedef quint16 BlockSize; /**< Block is comprised of several chunks */
typedef std::unique_ptr<QTcpSocket> QTcpSocketPtr;
typedef std::unique_ptr<QEventLoop> QEventLoopPtr;
typedef std::shared_ptr<ChunkCollector> FileChunkCollectorPtr;

//////////////////////////////////////////////////
///               ChunkCollector               ///
//////////////////////////////////////////////////

/**
* Collects chunks and their number
*/

class ChunkCollector
{
public:
    ChunkCollector();

    void addChunck(const quint64 chunck);
	const QHash<Chunk, quint64> getChunks() const;
    void setFileName(const QString fileName);
    QString getFileName() const;   
    void setFileSize(const quint64& fileSize);
    quint64 getFileSize() const;

private:
    QString mFileName;
    QHash<Chunk, quint64> mChunks;
};

//////////////////////////////////////////////////
///                 ChunckReceiver             ///
//////////////////////////////////////////////////

/**
*  Receives data from Loader and stores it in ChunkCollector
*/

class ChunckReceiver : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit ChunckReceiver(int descriptor, FileChunkCollectorPtr collector, QObject *parent = 0);

    void run();
    const FileChunkCollectorPtr getCollector() const;

signals:
    void error(QTcpSocket::SocketError socketerror);
    void finished();

public slots:
    void onDisconnect();
    void onReadyRead();

private:
	FileChunkCollectorPtr mCollector;

    int mSocketDescriptor;
	QTcpSocketPtr mSocket;
    QEventLoopPtr mEventLoop;
    BlockSize mNextBlockSize; 
	quint64 mBytesLeft;       
};

//////////////////////////////////////////////////
///                  Analyzer                  ///
//////////////////////////////////////////////////

/**
* @file analyzer.h
* @brief Aside from it's server role, owns all collectors of chinks and prints the final result
*/

class Analyzer : public QTcpServer
{
    Q_OBJECT

public:
	explicit Analyzer(std::ostream* stream = &std::cout, QObject* parent = 0);
    bool startServer(const quint16 port);

private: 
	void print();

private slots:
    void onReadCompleted();

protected:
    void incomingConnection(int socketDescriptor);

signals:
    void finished();

private:
    QList<FileChunkCollectorPtr> mCollectors;
    QHash<Chunk, QPair<QStringList, quint64>> mFiles;   /**< value is the list of files that contain a chunk and it's total number */
	std::ostream* mOutputStream;  /**< A stream to output the result. Can be file or cout */
};

#endif // ANALYZER_H


