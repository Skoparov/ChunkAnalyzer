#include "analyzer.h"

//////////////////////////////////////////////////
///               ChunkCollector               ///
//////////////////////////////////////////////////

ChunkCollector::ChunkCollector()
{

}

void ChunkCollector::addChunck(const quint64 chunck)
{
    quint64& chunckCount = mChunks[chunck];
    chunckCount++;
}

void ChunkCollector::setFileName(const QString fileName)
{
    mFileName = fileName;
}

QString ChunkCollector::getFileName() const
{
    return mFileName;
}

const QHash<quint64, quint64> ChunkCollector::getChunks() const
{
    return mChunks;
}

//////////////////////////////////////////////////
///                 ChunckReceiver             ///
//////////////////////////////////////////////////

ChunckReceiver::ChunckReceiver(qintptr descriptor, FileChunkCollectorPtr collector, QObject *parent) :
    QObject(parent),  
	mNextBlockSize(0),
    mSocketDescriptor(descriptor),
    mCollector(collector)
{

}

void ChunckReceiver::run()
{
    if(mCollector == nullptr){
        return;
    }

	mEventLoop = QEventLoopPtr(new QEventLoop());
	mSocket = QTcpSocketPtr(new QTcpSocket());
    connect(mSocket.get(),
            SIGNAL(disconnected()),
            this,
            SLOT(onDisconnect()),Qt::DirectConnection);

    connect(mSocket.get(),
            SIGNAL(readyRead()),
            this,
            SLOT(onReadyRead()), Qt::DirectConnection);

    if(!mSocket->setSocketDescriptor(mSocketDescriptor)){
            emit error(mSocket->error());
    }

    mEventLoop->exec();

    emit finished();
}

void ChunckReceiver::onReadyRead()
{
	QDataStream in(mSocket.get());  
    if(!mNextBlockSize)
    {
        if(mSocket->bytesAvailable() < sizeof(mNextBlockSize)){
            return;
        }

        in >> mNextBlockSize;
    }

    if(mSocket->bytesAvailable() < mNextBlockSize){
        return;
    }	

    if(!mCollector->getFileName().isEmpty())
    {
        for (int chunckNumber = 0; chunckNumber < mNextBlockSize/sizeof(quint64); ++chunckNumber)
        {
            Chunk chunk;
            in >> chunk;
            mCollector->addChunck(chunk);
            mBytesLeft -= sizeof(Chunk);
        }
    }
    else //this is the first transmission transmission comprised of the name of a file and it's size
    {
        quint64 fileSize;
        in>>fileSize;
        mBytesLeft = fileSize;

        QByteArray name = mSocket->read(mNextBlockSize - sizeof(quint64));
        QString fileName = QString::fromLocal8Bit(name.data());
        qDebug()<<QString("%1 | %2 bytes").arg(fileName).arg(QString::number(fileSize));

        mCollector->setFileName(fileName);     
    }
   		
	std::cout << QString("%1 : %2 bytes left\n")
		.arg(mCollector->getFileName())
		.arg(mBytesLeft).toStdString();

	//response is the number of bytes received by the module
	QByteArray response;
	QDataStream responseStream(&response, QIODevice::WriteOnly);
	responseStream << mNextBlockSize;
	mSocket->write(response);

	mNextBlockSize = 0;       
}

const FileChunkCollectorPtr ChunckReceiver::getCollector() const
{
	return mCollector;
}

void ChunckReceiver::onDisconnect()
{
	mEventLoop->exit();
}

//////////////////////////////////////////////////
///                  Analyzer                  ///
//////////////////////////////////////////////////

Analyzer::Analyzer(std::ostream* stream, QObject *parent) : QTcpServer(parent), mOutputStream(stream)
{
    QThreadPool::globalInstance()->setExpiryTimeout(0); //to make threads exit right after the job is done
}

bool Analyzer::startServer(const quint16 port)
{
    return listen(QHostAddress::Any, port);
}

void Analyzer::onReadCompleted()
{      
	// if there is no more ongoing transmissions, printing
    if(QThreadPool::globalInstance()->activeThreadCount() == 0){
		print();
    }
}

void Analyzer::print()
{		
	for (auto collector : mCollectors)
	{
		QString name = collector->getFileName();
		const QHash<quint64, quint64> chuncks = collector->getChunks();

		for (auto chunkIter = chuncks.begin(); chunkIter != chuncks.end(); ++chunkIter)
		{
			auto& chunkData = mFiles[chunkIter.key()];

			chunkData.second += *chunkIter;
			if (!chunkData.first.contains(name)){
				chunkData.first.append(name);
			}
		}
	}

	for (auto iter = mFiles.begin(); iter != mFiles.end(); ++iter)
	{
		const QPair<QStringList, quint64>& data = *iter;
		QString currChunck = QString("%1").arg(QString::number(iter.key(), 16).toUpper(), 16, QChar('0'));

		QString line = QString("[%1] %2 %3\n")
			.arg(currChunck)
			.arg(data.second)
			.arg(data.first.join(" "));

		(*mOutputStream) << line.toStdString();
	}

	emit finished();
}

void Analyzer::incomingConnection(qintptr socketDescriptor)
{  
    FileChunkCollectorPtr collector (new ChunkCollector);
    ChunckReceiver* connection = new ChunckReceiver(socketDescriptor, collector);
    QThreadPool::globalInstance()->start(connection);

    connect(connection, SIGNAL(finished()), this, SLOT(onReadCompleted()));
    mCollectors.append(collector);
}
