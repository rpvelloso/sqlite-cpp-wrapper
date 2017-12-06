#ifndef SQLITE_CPP_H_
#define SQLITE_CPP_H_

#include <exception>
#include <memory>

#include "sqlite3.h"

void closeDB(sqlite3 *h) {
	if (h != nullptr)
		sqlite3_close(h);
}

class SQLiteDB {
friend class SQLiteQuery;
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
private:
	std::string filename;
	std::unique_ptr<sqlite3, decltype(&closeDB)> dbHandle;
};

void finalizeStmt(sqlite3_stmt *stmt) {
	if (stmt != nullptr)
		sqlite3_finalize(stmt);
}

class SQLiteQuery {
	friend class SQLiteResultBase;
public:
	SQLiteQuery(SQLiteDB &db) : 
		db(db),
		stmt(nullptr, finalizeStmt) {
	
	};
	
	SQLiteQuery(SQLiteDB &db, const std::string &query) : 
		db(db), 
		query(query),
		stmt(nullptr, finalizeStmt) {
			
		prepare();
	};
	
	SQLiteQuery &operator=(const std::string &query) {
		this->query = query;
		prepare();
		return *this;
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
	SQLiteDB &db;
	std::string query;
	std::unique_ptr<sqlite3_stmt, decltype(&finalizeStmt)> stmt;
	
	void prepare() {
		sqlite3_stmt *stmtHandle;
		
		auto res = sqlite3_prepare(
			db.dbHandle.get(),
			query.c_str(),
			query.size(),
			&stmtHandle,
			nullptr);
		
		if (res != SQLITE_OK)
			throw std::runtime_error(sqlite3_errstr(res));
				
		stmt.reset(stmtHandle);
	}
};

template<class T>
T getColumn(sqlite3_stmt *stmt, int c) {
	T result;
	throw std::runtime_error("invalid or unhandled data type");
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
		return "";
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
	SQLiteResultBase(SQLiteQuery &query) : query(query) {};
	SQLiteResultBase(SQLiteResultBase &&) = default;
	SQLiteResultBase(const SQLiteResultBase &) = default;
	SQLiteResultBase &operator=(const SQLiteResultBase &) = default;
	SQLiteResultBase &operator=(SQLiteResultBase &&) = default;
	virtual ~SQLiteResultBase() {};
protected:
	sqlite3_stmt *getQueryStmt() {
		return query.stmt.get();
	};
private:
	SQLiteQuery &query;
};

template<class ... Types>
class SQLiteResult : public SQLiteResultBase {
public:
	SQLiteResult(SQLiteQuery &query) : SQLiteResultBase(query) {};
	
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