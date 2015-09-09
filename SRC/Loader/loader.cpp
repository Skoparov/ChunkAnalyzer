#include "loader.h"

//////////////////////////////////////////////////
///                 FileThread                 ///
//////////////////////////////////////////////////

Loader::Loader(const QString& addr, const quint16& port) : mAddr(addr), mPort(port)
{
	QThreadPool::globalInstance()->setExpiryTimeout(0); //to make threads exit right after the job is done
}

bool Loader::manageDirs(const QStringList dirs)
{
    QDir dir;	
    for(auto dirPath : dirs)
    {
        dir.setPath(dirPath);
        QStringList fileNames = dir.entryList(QDir::Files);	

        for(auto fileName : fileNames)
        {		
			std::cout << QString("Processing %1").arg(fileName).toStdString() <<std::endl;

            QString absFilePath = dir.absoluteFilePath(fileName);
            ChunkSender* sender = new ChunkSender(absFilePath, mAddr, mPort);
			
			connect(sender, SIGNAL(finished()), this, SLOT(sendFinished()));
            QThreadPool::globalInstance()->start(sender);
        }
    }

	if (QThreadPool::globalInstance()->activeThreadCount() == 0)
	{
		std::cout << "No files to process\n";
		return false;
	}

	return true;
}

void Loader::sendFinished()
{
	if (QThreadPool::globalInstance()->activeThreadCount() == 0) 
	{
		std::cout << "All files processed\n";
		emit finished();
	}
}

//////////////////////////////////////////////////
///                 FileThread                 ///
//////////////////////////////////////////////////

ChunkSender::ChunkSender(const QString filePath, const QString addr, const quint16& port, QObject *parent) :
    mChunksInBlock(4096),
    mPreviousBlockSize(0),
	mAddr(addr),
    mFilePath(filePath),
    mPort(port),
    QObject(parent)
{
	qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
}

void ChunkSender::run()
{
	mFile = QFilePtr(new QFile(mFilePath));
	mEventLoop = QEventLoopPtr(new QEventLoop);
	mSocket = QTcpSocketPtr(new QTcpSocket());

	connect(mSocket.get(), 
		   SIGNAL(error(QAbstractSocket::SocketError)), 
		   this, 
		   SLOT(onError(QAbstractSocket::SocketError)));

	connect(mSocket.get(), 
		   SIGNAL(connected()), 
		   this, 
		   SLOT(sendFileData()), Qt::DirectConnection);

	connect(mSocket.get(), 
		    SIGNAL(readyRead()), 
			this, 
			SLOT(onServerResponse()), Qt::DirectConnection);

    mSocket->connectToHost(mAddr, mPort);
    
    if(mFile->open(QIODevice::ReadOnly)){
		mEventLoop->exec();
    }
	else
	{
		std::cout << QString("%1 : Cannot open file. Transmission aborted\n")
			.arg(mFilePath.split("/").last()).toStdString();
	}  

	mFile->close();  
	emit finished();
}

void ChunkSender::sendFileData()
{	
	//The frist transmission comprised of file name and it's size

    QString fileNameStr = mFilePath.split("/").last();
    QByteArray fileName = fileNameStr.toLocal8Bit();

    QByteArray fileSize;
    QDataStream out(&fileSize, QIODevice::WriteOnly);			
    out<<quint64(mFile->size());
		
	write(fileSize + fileName);
}

void ChunkSender::onServerResponse()
{
	// server response is the number of bytes it has received. 
	// A small check to be sure everything is transmitted in due course	

	QDataStream in(mSocket.get());
	if (!mSocket->bytesAvailable() == sizeof(BlockSize)){
		return;
	}

	BlockSize response;
	in >> response;

	if (response != mPreviousBlockSize)
	{
		std::cout << QString("%1 : Error sending data. Transmission aborted\n")
			.arg(mFilePath.split("/").last()).toStdString();

		mEventLoop->exit();
		return;
	}

	// if bytes available to read is less than the chunk's size, we skip then
	// and cosider the transmission completed
	if (mFile->bytesAvailable() < sizeof(Chunk))
	{
		std::cout << QString("%1 : Transmission complete\n")
			.arg(mFilePath.split("/").last()).toStdString();

		mEventLoop->exit();
		return;
	}

	//if everything is ok, send next block
	sendNextBlock();
}

void ChunkSender::sendNextBlock()
{   
    mPreviousBlockSize = 0;
    QByteArray block;
    QDataStream blockStream(&block, QIODevice::WriteOnly);
    blockStream<<mPreviousBlockSize;

    for(int chunckNumber = 0; chunckNumber < mChunksInBlock; ++ chunckNumber)
    {
		int bytesAval = mFile->bytesAvailable();

        if(mFile->bytesAvailable() >= sizeof(Chunk)){
            block += mFile->read(sizeof(Chunk));
        }
		else{			
			break;
		}
    }

    if(block.size() > sizeof(mPreviousBlockSize))
    {
        blockStream.device()->seek(0);
        mPreviousBlockSize = block.size() - sizeof(mPreviousBlockSize);
        blockStream<<mPreviousBlockSize;
        mSocket->write(block);
    }  
}

void ChunkSender::write(const QByteArray data)
{
	QByteArray header;
	QDataStream out(&header, QIODevice::WriteOnly);
	out << BlockSize(data.size());

	BlockSize size = header.size() + data.size();
	if (mSocket->write(header + data) == size){
		mPreviousBlockSize = data.size();
	}
	else
	{
		std::cout << QString("%1 : Error sending data. Transmission aborted\n")
			.arg(mFilePath.split("/").last()).toStdString();

		mEventLoop->exit();
	}
}

void ChunkSender::onError(QAbstractSocket::SocketError error)
{
	QString errorType = (error == QAbstractSocket::HostNotFoundError ? "Host not found" :
		error == QAbstractSocket::ConnectionRefusedError ? "Connection timed out" :
		error == QAbstractSocket::RemoteHostClosedError ? "Connection refused" : "Unknown error");

	QString errorText = QString("Error: %1").arg(errorType);

	std::cout << QString("%1 : Error connecting to server (%2)\n")
		.arg(mFilePath.split("/").last())
		.arg(errorText).toStdString();

	mEventLoop->exit();
}





