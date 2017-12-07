#ifndef SQLITE_CPP_H_
#define SQLITE_CPP_H_

#include <exception>
#include <memory>

#include "sqlite3.h"

void closeDB(sqlite3 *h) {
	if (h != nullptr)
		sqlite3_close(h);
}

template<class ... Types>
class SQLiteQuery;

class SQLiteDB {
friend class SQLiteQueryBase;
public:
	SQLiteDB(const std::string &filename) :
		filename(filename),
		dbHandle(nullptr, closeDB) {

		sqlite3 *handle;

		auto res = sqlite3_open(this->filename.c_str(), &handle);
		if (res != SQLITE_OK)
			throw std::runtime_error(sqlite3_errstr(res));
		dbHandle.reset(handle);
	};

	template<class ... Types>
	SQLiteQuery<Types...> makeQuery(const std::string &queryString, const Types& ... values);

	SQLiteQuery<> makeQuery(const std::string &queryString);


private:
	std::string filename;
	std::unique_ptr<sqlite3, decltype(&closeDB)> dbHandle;
};

template<class T>
int bindValue(sqlite3_stmt *stmt, int c, const T& value) {
	throw std::runtime_error("invalid or unhandled data type - bind");
	return 0;
};

template<>
int bindValue<int>(sqlite3_stmt *stmt, int c, const int &value) {
	return sqlite3_bind_int(stmt, c, value);
}

template<>
int bindValue<sqlite_int64>(sqlite3_stmt *stmt, int c, const sqlite_int64 &value) {
	return sqlite3_bind_int64(stmt, c, value);
}

template<>
int bindValue<double>(sqlite3_stmt *stmt, int c, const double &value) {
	return sqlite3_bind_double(stmt, c, value);
}

template<>
int bindValue<std::string>(sqlite3_stmt *stmt, int pos, const std::string &value) {
	return sqlite3_bind_text(stmt, pos, value.c_str(), -1, SQLITE_TRANSIENT);
}

template<class T>
void _bindValues(sqlite3_stmt *stmt, int k, const T& first) {
	auto res = bindValue<T>(stmt, k, first);
	if (res != SQLITE_OK)
		throw std::runtime_error(sqlite3_errstr(res));
}

template<class T, class ...Types>
void _bindValues(sqlite3_stmt *stmt, int k, const T& first, const Types&...args) {
	auto res = bindValue<T>(stmt, k, first);
	if (res != SQLITE_OK)
		throw std::runtime_error(sqlite3_errstr(res));
	_bindValues(stmt, k+1, args...);
}

void finalizeStmt(sqlite3_stmt *stmt) {
	if (stmt != nullptr)
		sqlite3_finalize(stmt);
}

class SQLiteQueryBase {
	friend class SQLiteResultBase;
public:
	SQLiteQueryBase(SQLiteDB &db) : db(db), stmt(nullptr, finalizeStmt) {};
	SQLiteQueryBase(SQLiteQueryBase &&) = default;
	SQLiteQueryBase(const SQLiteQueryBase &) = default;
	virtual SQLiteQueryBase &operator=(const SQLiteQueryBase &) = default;
	virtual SQLiteQueryBase &operator=(SQLiteQueryBase &&) = default;
	virtual ~SQLiteQueryBase() {};
private:
	SQLiteDB &db;
protected:
	std::unique_ptr<sqlite3_stmt, decltype(&finalizeStmt)> stmt;
	sqlite3 *getDBHandle() {
		return db.dbHandle.get();
	};
};

template<class ... Types>
class SQLiteQuery : public SQLiteQueryBase {
public:
	SQLiteQuery(SQLiteDB &db) : 
		SQLiteQueryBase(db) {
	};
	
	SQLiteQuery(SQLiteDB &db, const std::string &query) : 
		SQLiteQueryBase(db),
		query(query) {
			
		prepare();
	};
	
	SQLiteQuery &operator=(const std::string &query) {
		this->query = query;
		prepare();
		return *this;
	}
	
	void bindValues(const Types&... args) {
		_bindValues(stmt.get(), 1, args...);
	}

	void bind(int pos, int value) {
		auto res = sqlite3_bind_int(stmt.get(), pos, value);
		if (res != SQLITE_OK)
			throw std::runtime_error(sqlite3_errstr(res));
	}
	
	void bind(int pos, sqlite3_int64 value) {
		auto res = sqlite3_bind_int64(stmt.get(), pos, value);
		if (res != SQLITE_OK)
			throw std::runtime_error(sqlite3_errstr(res));
	}
	
	void bind(int pos, double value) {
		auto res = sqlite3_bind_double(stmt.get(), pos, value);
		if (res != SQLITE_OK)
			throw std::runtime_error(sqlite3_errstr(res));
	}

	void bind(int pos, const std::string &value) {
		auto res = sqlite3_bind_text(stmt.get(), pos, value.c_str(), -1, SQLITE_TRANSIENT);
		if (res != SQLITE_OK)
			throw std::runtime_error(sqlite3_errstr(res));
	}
	
	void execute() {
		auto res = sqlite3_step(stmt.get());
		if (res != SQLITE_OK && res != SQLITE_DONE)
			throw std::runtime_error(sqlite3_errstr(res));
	};
	
	void reset() {
		auto res = sqlite3_reset(stmt.get());
		if (res != SQLITE_OK)
			throw std::runtime_error(sqlite3_errstr(res));
	};
private:
	std::string query;
	
	void prepare() {
		sqlite3_stmt *stmtHandle;
		
		auto res = sqlite3_prepare(
			getDBHandle(),
			query.c_str(),
			query.size(),
			&stmtHandle,
			nullptr);
		
		if (res != SQLITE_OK)
			throw std::runtime_error(sqlite3_errstr(res));
				
		stmt.reset(stmtHandle);
	}
};

template<class ... Types>
SQLiteQuery<Types...> SQLiteDB::makeQuery(const std::string &queryString, const Types& ... values) {
	SQLiteQuery<Types...> query(*this, queryString);
	query.bindValues(values...);
	return query;
}

SQLiteQuery<> SQLiteDB::makeQuery(const std::string &queryString) {
	SQLiteQuery<> query(*this, queryString);
	return query;
}


template<class T>
T getColumn(sqlite3_stmt *stmt, int c) {
	T result;
	throw std::runtime_error("invalid or unhandled data type - column");
	return result;
};

template<>
int getColumn<int>(sqlite3_stmt *stmt, int c) {
	return sqlite3_column_int(stmt, c);
}

template<>
sqlite_int64 getColumn<sqlite_int64>(sqlite3_stmt *stmt, int c) {
	return sqlite3_column_int64(stmt, c);
}

template<>
double getColumn<double>(sqlite3_stmt *stmt, int c) {
	return sqlite3_column_double(stmt, c);
}

template<>
std::string getColumn<std::string>(sqlite3_stmt *stmt, int c) {
	auto pStr = (const char *)sqlite3_column_text(stmt, c);
	auto len = sqlite3_column_bytes(stmt, c);
	if (pStr == nullptr)
		return std::string();
	return std::string((const char *)sqlite3_column_text(stmt, c), len);
}

template<class T>
void getRecord(sqlite3_stmt *stmt, int k, T &first) {
	first = getColumn<T>(stmt, k);
}

template<class T, class ...Types>
void getRecord(sqlite3_stmt *stmt, int k, T &first, Types&...args) {
	first = getColumn<T>(stmt, k);
	getRecord(stmt, k+1, args...);
}

class SQLiteResultBase {
public:
	SQLiteResultBase(SQLiteQueryBase &query) : query(query) {};
	SQLiteResultBase(SQLiteResultBase &&) = default;
	SQLiteResultBase(const SQLiteResultBase &) = default;
	virtual SQLiteResultBase &operator=(const SQLiteResultBase &) = default;
	virtual SQLiteResultBase &operator=(SQLiteResultBase &&) = default;
	virtual ~SQLiteResultBase() {};
protected:
	sqlite3_stmt *getQueryStmt() {
		return query.stmt.get();
	};
private:
	SQLiteQueryBase &query;
};

template<class ... Types>
class SQLiteResult : private SQLiteResultBase {
public:
	SQLiteResult(SQLiteQueryBase &query) : SQLiteResultBase(query) {};
	
	void fetch(Types&...args) {
		getRecord(getQueryStmt(), 0, args...);
	}
	
	bool next() {
		auto res = sqlite3_step(getQueryStmt());
		if (res == SQLITE_ROW)
			return true;
		if (res == SQLITE_DONE)
			return false;
		
		throw std::runtime_error(sqlite3_errstr(res));
	};
};

#endif // SQLITE_CPP_H_
