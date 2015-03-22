/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
   used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#ifndef NIFMODEL_H
#define NIFMODEL_H

#include "basemodel.h" // Inherited

#include <QHash>
#include <QReadWriteLock>
#include <QStack>
#include <QStringList>
#include <QUndoCommand>
#include <QUndoStack>


//! @file nifmodel.h NifModel, NifModelEval, ChangeValueCommand, ToggleCheckBoxListCommand

//! The main data model for the NIF file.
class NifModel final : public BaseModel
{
	Q_OBJECT

	friend class NifXmlHandler;
	friend class NifModelEval;
	friend class NifOStream;

public:
	NifModel( QObject * parent = 0 );

	//! Find and parse the XML file
	static bool loadXML();

	//! When creating NifModels from outside the main thread protect them with a QReadLocker
	static QReadWriteLock XMLlock;

	// QAbstractItemModel

	QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const override final;
	bool setData( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole ) override final;
	bool removeRows( int row, int count, const QModelIndex & parent ) override final;

	// end QAbstractItemModel

	// BaseModel
	
	void clear() override final;
	bool load( QIODevice & device ) override final;
	bool save( QIODevice & device ) const override final;

	QString getVersion() const override final { return version2string( version ); }
	quint32 getVersionNumber() const override final { return version; }

	template <typename T> T get( const QModelIndex & index ) const;
	template <typename T> bool set( const QModelIndex & index, const T & d );

	template <typename T> T get( const QModelIndex & parent, const QString & name ) const;
	template <typename T> bool set( const QModelIndex & parent, const QString & name, const T & v );

	// end BaseModel

	//! Load from QIODevice and index
	bool load( QIODevice & device, const QModelIndex & );
	//! Save to QIODevice and index
	bool save( QIODevice & device, const QModelIndex & ) const;
	//! Resets the model to its original state in any attached views.
	void reset();

	//! Loads a model and maps links
	bool loadAndMapLinks( QIODevice & device, const QModelIndex &, const QMap<qint32, qint32> & map );
	//! Loads the header from a filename
	bool loadHeaderOnly( const QString & fname );

	//! Returns the the estimated file offset of the model index
	int fileOffset( const QModelIndex & ) const;

	//! Returns the estimated file size of the model index
	int blockSize( const QModelIndex & ) const;
	//! Returns the estimated file size of the item
	int blockSize( NifItem * parent ) const;
	//! Returns the estimated file size of the stream
	int blockSize( NifItem * parent, NifSStream & stream ) const;

	/*! Checks if the specified file contains the specified block ID in its header and is of the specified version
	 *
	 * Note that it will not open the full file to look for block types, only the header
	 *
	 * @param filepath	The NIF to check
	 * @param blockId	The block to check for
	 * @param version	The version to check for
	 */
	bool earlyRejection( const QString & filepath, const QString & blockId, quint32 version );

	//! Returns the model index of the NiHeader
	QModelIndex getHeader() const;
	//! Updates the header infos ( num blocks etc. )
	void updateHeader();

	//! Returns the model index of the NiFooter
	QModelIndex getFooter() const;
	//! Updates the footer info (num root links etc. )
	void updateFooter();

	//! Set delayed updating of model links
	bool holdUpdates( bool value );

	//! Insert or append ( row == -1 ) a new NiBlock
	QModelIndex insertNiBlock( const QString & identifier, int row = -1, bool fast = false );
	//! Remove a block from the list
	void removeNiBlock( int blocknum );
	//! Move a block in the list
	void moveNiBlock( int src, int dst );
	//! Return the block name
	QString getBlockName( const QModelIndex & ) const;
	//! Return the block type
	QString getBlockType( const QModelIndex & ) const;
	//! Return the block number
	int getBlockNumber( const QModelIndex & ) const;

	/*! Get the NiBlock at a given index
	 *
	 * @param idx	The model index to get the parent of
	 * @param name	Optional: the type to check for
	 */
	QModelIndex getBlock( const QModelIndex & idx, const QString & name = QString() ) const;

	/*! Get the NiBlock at a given index
	 *
	 * @param x		The integer index of the block
	 * @param name	Optional: the type to check for
	 */
	QModelIndex getBlock( int x, const QString & name = QString() ) const;

	//! Returns the parent block or header
	QModelIndex getBlockOrHeader( const QModelIndex & ) const;

	//! Get the number of NiBlocks
	int getBlockCount() const;

	/*! Check if a given index is a NiBlock
	 *
	 * @param index	The index to check
	 * @param name	Optional: the type to check for
	 */
	bool isNiBlock( const QModelIndex & index, const QString & name = QString() ) const;

	/*! Check if a given index is a NiBlock
	 *
	 * @param index	The index to check
	 * @param names	A list of names to check
	 */
	bool isNiBlock( const QModelIndex & index, const QStringList & names ) const;

	//! Returns a list with all known NiXXX ids (<niobject abstract="0">)
	static QStringList allNiBlocks();
	//! Determine if a value is a NiBlock identifier (<niobject abstract="0">).
	static bool isNiBlock( const QString & name );
	//! Reorders the blocks according to a list of new block numbers
	void reorderBlocks( const QVector<qint32> & order );
	//! Moves all niblocks from this nif to another nif, returns a map which maps old block numbers to new block numbers
	QMap<qint32, qint32> moveAllNiBlocks( NifModel * targetnif, bool update = true );
	//! Convert a block from one type to another
	void convertNiBlock( const QString & identifier, const QModelIndex & index, bool fast = false );

	void insertType( const QModelIndex & parent, const NifData & data, int atRow );

	QList<int> getRootLinks() const;
	QList<int> getChildLinks( int block ) const;
	QList<int> getParentLinks( int block ) const;

	/*! Get parent
	 * @return	Parent block number or -1 if there are zero or multiple parents.
	 */
	int getParent( int block ) const;

	/*! Get parent
	 * @return	Parent block number or -1 if there are zero or multiple parents.
	 */
	int getParent( const QModelIndex & index ) const;

	//! Is it a child or parent link?
	bool isLink( const QModelIndex & index, bool * ischildLink = 0 ) const;
	//! Return a block number if the index is a valid link
	qint32 getLink( const QModelIndex & index ) const;

	/*! Get the block number of a link
	 *
	 * @param parent	The parent of the link
	 * @param name		The name of the link
	 */
	int getLink( const QModelIndex & parent, const QString & name ) const;
	QVector<qint32> getLinkArray( const QModelIndex & array ) const;
	QVector<qint32> getLinkArray( const QModelIndex & parent, const QString & name ) const;
	bool setLink( const QModelIndex & index, qint32 l );
	bool setLink( const QModelIndex & parent, const QString & name, qint32 l );
	bool setLinkArray( const QModelIndex & array, const QVector<qint32> & links );
	bool setLinkArray( const QModelIndex & parent, const QString & name, const QVector<qint32> & links );

	void mapLinks( const QMap<qint32, qint32> & map );

	//! Is name a compound type?
	static bool isCompound( const QString & name );
	//! Is name an ancestor identifier (<niobject abstract="1">)?
	static bool isAncestor( const QString & name );
	//! Is name a NiBlock identifier (<niobject abstract="0"> or <niobject abstract="1">)?
	bool isAncestorOrNiBlock( const QString & name ) const override final;
	//! Returns true if name inherits ancestor.
	bool inherits( const QString & name, const QString & ancestor ) const override final;
	// returns true if the block containing index inherits ancestor
	bool inherits( const QModelIndex & index, const QString & ancestor ) const;

	//! Is this version supported?
	static bool isVersionSupported( quint32 );

	static QString version2string( quint32 );
	static quint32 version2number( const QString & );

	//! Check whether the current NIF file version lies in the range [since, until]
	bool checkVersion( quint32 since, quint32 until ) const;

	quint32 getUserVersion() const { return get<int>( getHeader(), "User Version" ); }

	QString string( const QModelIndex & index, bool extraInfo = false ) const;
	QString string( const QModelIndex & index, const QString & name, bool extraInfo = false ) const;

	bool assignString( const QModelIndex & index, const QString & string, bool replace = false );
	bool assignString( const QModelIndex & index, const QString & name, const QString & string, bool replace = false );

	//! Create and return delegate for SpellBook
	static QAbstractItemDelegate * createDelegate( class SpellBook * );

	//! Undo Stack for changes to NifModel
	QUndoStack * undoStack;

public slots:
	void updateSettings();

signals:
	void linksChanged();
	void lodSliderChanged( bool ) const;
	void beginUpdateHeader();

protected:
	// BaseModel

	NifItem * getItem( NifItem * parent, const QString & name ) const override final;

	bool setItemValue( NifItem * item, const NifValue & v ) override final;

	bool updateArrayItem( NifItem * array, bool fast ) override final;

	QString ver2str( quint32 v ) const override final { return version2string( v ); }
	quint32 str2ver( QString s ) const override final { return version2number( s ); }

	bool evalVersion( NifItem * item, bool chkParents = false ) const override final;

	bool setHeaderString( const QString & ) override final;

	template <typename T> T get( NifItem * parent, const QString & name ) const;
	template <typename T> T get( NifItem * item ) const;
	template <typename T> bool set( NifItem * parent, const QString & name, const T & d );
	template <typename T> bool set( NifItem * item, const T & d );

	// end BaseModel

	bool load( NifItem * parent, NifIStream & stream, bool fast = true );
	bool save( NifItem * parent, NifOStream & stream ) const;
	bool fileOffset( NifItem * parent, NifItem * target, NifSStream & stream, int & ofs ) const;

	NifItem * getHeaderItem() const;
	NifItem * getFooterItem() const;
	NifItem * getBlockItem( int ) const;

	int getBlockNumber( NifItem * item ) const;
	bool itemIsLink( NifItem * item, bool * ischildLink = 0 ) const;

	void insertAncestor( NifItem * parent, const QString & identifier, int row = -1 );
	void insertType( NifItem * parent, const NifData & data, int row = -1 );
	NifItem * insertBranch( NifItem * parent, const NifData & data, int row = -1 );

	bool updateByteArrayItem( NifItem * array, bool fast );
	bool updateArrays( NifItem * parent, bool fast );

	void updateLinks( int block = -1 );
	void updateLinks( int block, NifItem * parent );
	void checkLinks( int block, QStack<int> & parents );
	void adjustLinks( NifItem * parent, int block, int delta );
	void mapLinks( NifItem * parent, const QMap<qint32, qint32> & map );

	static void updateStrings( NifModel * src, NifModel * tgt, NifItem * item );
	bool assignString( NifItem * parent, const QString & string, bool replace = false );

	//! NIF file version
	quint32 version;

	QHash<int, QList<int> > childLinks;
	QHash<int, QList<int> > parentLinks;
	QList<int> rootLinks;

	bool lockUpdates;

	enum UpdateType
	{
		utNone   = 0,
		utHeader = 0x1,
		utLinks  = 0x2,
		utFooter = 0x4,
		utAll = 0x7
	};
	UpdateType needUpdates;

	void updateModel( UpdateType value = utAll );

	//! Parse the XML file using a NifXmlHandler
	static QString parseXmlDescription( const QString & filename );

	// XML structures
	static QList<quint32> supportedVersions;
	static QHash<QString, NifBlock *> compounds;
	static QHash<QString, NifBlock *> blocks;

private:
	struct Settings
	{
		QString startupVersion;
	} cfg;
};


//! Helper class for evaluating condition expressions
class NifModelEval
{
public:
	NifModelEval( const NifModel * model, const NifItem * item );

	QVariant operator()( const QVariant & v ) const;
private:
	const NifModel * model;
	const NifItem * item;
};


class ChangeValueCommand : public QUndoCommand
{
public:
	ChangeValueCommand( const QModelIndex & index, const QVariant & value, const QString & valueString, const QString & valueType, NifModel * model );
	void redo() override;
	void undo() override;
private:
	NifModel * nif;
	QVariant newValue, oldValue;
	QModelIndex idx;
};


class ToggleCheckBoxListCommand : public QUndoCommand
{
public:
	ToggleCheckBoxListCommand( const QModelIndex & index, const QVariant & value, const QString & valueType, NifModel * model );
	void redo() override;
	void undo() override;
private:
	NifModel * nif;
	QVariant newValue, oldValue;
	QModelIndex idx;
};


// Inlines


inline QStringList NifModel::allNiBlocks()
{
	QStringList lst;
	for ( NifBlock * blk : blocks ) {
		if ( !blk->abstract )
			lst.append( blk->id );
	}
	return lst;
}

inline bool NifModel::isAncestorOrNiBlock( const QString & name ) const
{
	return blocks.contains( name );
}

inline bool NifModel::isNiBlock( const QString & name )
{
	NifBlock * blk = blocks.value( name );
	return blk && !blk->abstract;
}

inline bool NifModel::isAncestor( const QString & name )
{
	NifBlock * blk = blocks.value( name );
	return blk && blk->abstract;
}

inline bool NifModel::isCompound( const QString & name )
{
	return compounds.contains( name );
}

inline bool NifModel::isVersionSupported( quint32 v )
{
	return supportedVersions.contains( v );
}

inline QList<int> NifModel::getRootLinks() const
{
	return rootLinks;
}

inline QList<int> NifModel::getChildLinks( int block ) const
{
	return childLinks.value( block );
}

inline QList<int> NifModel::getParentLinks( int block ) const
{
	return parentLinks.value( block );
}

inline bool NifModel::itemIsLink( NifItem * item, bool * isChildLink ) const
{
	if ( isChildLink )
		*isChildLink = ( item->value().type() == NifValue::tLink );

	return item->value().isLink();
}

inline bool NifModel::checkVersion( quint32 since, quint32 until ) const
{
	return (( since == 0 || since <= version ) && ( until == 0 || version <= until ));
}


// Templates


template <typename T> inline T NifModel::get( const QModelIndex & index ) const
{
	return BaseModel::get<T>( index );
}

template <typename T> inline T NifModel::get( NifItem * item ) const
{
	return BaseModel::get<T>( item );
}

template <typename T> inline T NifModel::get( NifItem * parent, const QString & name ) const
{
	return BaseModel::get<T>( parent, name );
}

template <typename T> inline T NifModel::get( const QModelIndex & parent, const QString & name ) const
{
	return BaseModel::get<T>( parent, name );
}

template <typename T> inline bool NifModel::set( const QModelIndex & index, const T & d )
{
	return BaseModel::set<T>( index, d );
}

template <typename T> inline bool NifModel::set( NifItem * item, const T & d )
{
	return BaseModel::set<T>( item, d );
}

template <typename T> inline bool NifModel::set( const QModelIndex & parent, const QString & name, const T & d )
{
	return BaseModel::set<T>( parent, name, d );
}

template <typename T> inline bool NifModel::set( NifItem * parent, const QString & name, const T & d )
{
	return BaseModel::set<T>( parent, name, d );
}

template <> inline QString NifModel::get( const QModelIndex & index ) const
{
	return this->string( index );
}

//template <> inline QString NifModel::get( NifItem * item ) const {
//	return this->string( this->item );
//}

//template <> inline QString NifModel::get( NifItem * parent, const QString & name ) const {
//	return this->string(parent, name);
//}

template <> inline QString NifModel::get( const QModelIndex & parent, const QString & name ) const
{
	return this->string( parent, name );
}

template <> inline bool NifModel::set( const QModelIndex & index, const QString & d )
{
	return this->assignString( index, d );
}

//template <> inline bool NifModel::set( NifItem * item, const QString & d ) {
//	return this->assignString( item, d );
//}

template <> inline bool NifModel::set( const QModelIndex & parent, const QString & name, const QString & d )
{
	return this->assignString( parent, name, d );
}

//template <> inline bool NifModel::set( NifItem * parent, const QString & name, const QString & d ) {
//	return this->assignString(parent, name, d);
//}
#endif
