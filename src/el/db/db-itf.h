#pragma once

namespace Ext { namespace DB {

class Sql {
public:
	static String AFXAPI Escape(RCString s);
};

ENUM_CLASS(DbType) {
	Null,
	Integer,
	Float,
	Blob,
	Text
} END_ENUM_CLASS(DbType);

interface IDataRecord : public Object {
	virtual Int32 GetInt32(int i) =0;
	virtual Int64 GetInt64(int i) =0;
	virtual double GetDouble(int i) =0;
	virtual String GetString(int i) =0;
	virtual ConstBuf GetBytes(int i) =0;
	virtual DbType GetFieldType(int i) =0;
	virtual int FieldCount() =0;
	virtual String GetName(int idx) =0;

	virtual int GetOrdinal(RCString name) {
		for (int i=0, n=FieldCount(); i<n; ++i)
			if (GetName(i) == name)
				return i;
		Throw(E_INVALIDARG);
	}

	virtual bool IsDBNull(int i) { return GetFieldType(i) == DbType::Null; }
};

interface IDataReader : public IDataRecord {
	virtual bool Read() =0;
};

interface IDbCommand : public Object {
	String CommandText;

	virtual IDbCommand& Bind(int column, std::nullptr_t) =0;
	virtual IDbCommand& Bind(int column, Int32 v) =0;
	virtual IDbCommand& Bind(int column, Int64 v) =0;
	virtual IDbCommand& Bind(int column, double v) =0;
	virtual IDbCommand& Bind(int column, const ConstBuf& mb, bool bTransient = true) =0;
	virtual IDbCommand& Bind(int column, RCString s) =0;

	virtual IDbCommand& Bind(RCString parname, std::nullptr_t) =0;
	virtual IDbCommand& Bind(RCString parname, Int32 v) =0;
	virtual IDbCommand& Bind(RCString parname, Int64 v) =0;
	virtual IDbCommand& Bind(RCString parname, double v) =0;
	virtual IDbCommand& Bind(RCString parname, const ConstBuf& mb, bool bTransient = true) =0;
	virtual IDbCommand& Bind(RCString parname, RCString s) =0;

	virtual void Dispose() =0;
	virtual void ExecuteNonQuery() =0;
	virtual String ExecuteScalar() =0;
};

interface ITransactionable {
public:
	virtual void BeginTransaction() =0;
	virtual void Commit() =0;
	virtual void Rollback() =0;
};

class TransactionScope : noncopyable {
public:
	ITransactionable& m_db;

	TransactionScope(ITransactionable& db)
		:	m_db(db)
	{
		m_db.BeginTransaction();
	}

	~TransactionScope() {
		if (!m_bCommitted) {
			if (std::uncaught_exception())
				m_db.Rollback();
			else
				m_db.Commit();
		}
	}

	void Commit() {
		m_db.Commit();
		m_bCommitted = true;
	}
private:
	CBool m_bCommitted;
};

interface IDbConn : public Object {
	typedef IDbConn class_type;
public:
	virtual void Create(RCString file) =0;
	virtual void Open(RCString file, FileAccess fileAccess, FileShare share = FileShare::ReadWrite) =0;
	virtual void Close() =0;
	virtual ptr<IDbCommand> CreateCommand() =0;

	virtual void ExecuteNonQuery(RCString sql) {
		ptr<IDbCommand> cmd = CreateCommand();
		cmd->CommandText = sql;
		cmd->ExecuteNonQuery();
	}
	
	virtual Int64 get_LastInsertRowId() =0;
	DEFPROP_VIRTUAL_GET(Int64, LastInsertRowId);

	void Register(IDbCommand& cmd) { m_commands.insert(&cmd); }
	void Unregister(IDbCommand& cmd) { m_commands.erase(&cmd); }

protected:
	unordered_set<IDbCommand*> m_commands;

	void DisposeCommands() {
		EXT_FOR (IDbCommand *cmd, m_commands) {
			cmd->Dispose();
		}
		m_commands.clear();
	}
};

class DbException : public Exception {
	typedef Exception base;
public:
	DbException(HRESULT hr, RCString s)
		:	base(hr, s)
	{
	}
};

ENUM_CLASS(KVEnvFlags) {
	ReadOnly = 1,
	NoSync = 2
} END_ENUM_CLASS(KVEnvFlags);

ENUM_CLASS(CursorPos) {
	First,
	Next,
	Prev,
	Last,
	FindKey
} END_ENUM_CLASS(CursorPos);

ENUM_CLASS(KVStoreFlags) {
	NoOverwrite = 1
} END_ENUM_CLASS(KVStoreFlags);

}} // Ext::DB::


