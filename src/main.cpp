//Note: `mysql_config --cflags --libs` gives path to MySQL libs

#include <string>
#include <vector>
#include <iostream>

#include <QtCore>


#include <my_global.h>
#include <mysql.h>

#include "common.h"

class cache;

class record {
public:
    record() {}
    record *Prev, *Next;
    QString Key;
    QString Data;
    bool Modified;
    QMutex Mutex;

    Q_DISABLE_COPY(record)
};


class reference {
public:
    reference() : R(0) {}
    reference(record *R) : R(R) {}
    void release() {R->Mutex.unlock();}
    QString data() {return R->Data;}
    void setData(QString NewData) {
        R->Data = NewData;
        R->Modified = true;
    }

private:
    record *R;
};


class cache {
public:
    cache(char const *Host, char const *User, char const *Pass, char const *Scheme, bool New = false) {
        DB = mysql_init(NULL);
        if (mysql_real_connect(DB,Host,User,Pass,Scheme,0,NULL,0) == NULL) db_error();

        if (New) {
          if (mysql_query(DB, "DROP TABLE IF EXISTS records")) db_error();
          if (mysql_query(DB, "CREATE TABLE records("
                              "`key` VARCHAR(30) NOT NULL, PRIMARY KEY (`key`),"
                              "`data` TEXT)"))
              db_error();
        }

        //if (mysql_query(DB, "INSERT INTO records VALUES('K','V')"))
        //    db_error();


        record *Prev = 0;
        Records = map(I,seq(0,1024),
            record *R = new record;
            R->Modified = false;
            R->Prev = Prev;
            if (Prev) Prev->Next = R;
            Prev = R
        );
        Prev->Next = 0;
        LeastRecentlyUsed = Records.first();
        MostRecentlyUsed = Records.last();
    }

    ~cache() {
        flush();
        each(R, Records) delete R;
        mysql_close(DB);
    }

    void db_error() {
        fprintf(stderr, "%s\n", mysql_error(DB));
        mysql_close(DB);
        exit(1);
    }

    reference get(QString Key) {
        // make sure LRU entry isn't used whenwe update it
        LeastRecentlyUsed->Mutex.lock();
        Mutex.lock();
        LeastRecentlyUsed->Mutex.unlock();

        let(Found, Hash.find(Key));
        record *R = Found != Hash.end() ? *Found : 0;

        if (R) {
            if(R->Prev) R->Prev->Next = R->Next; // R was not LeastRecentlyUsed
            else LeastRecentlyUsed = R->Next; // R is no more LeastRecentlyUsed
        } else {
            R = LeastRecentlyUsed;
            LeastRecentlyUsed = R->Next;
            LeastRecentlyUsed->Prev = 0;
            flushRecord(R);

            Hash.erase(Hash.find(R->Key));
            R->Key = Key;
            Hash[R->Key] = R;
            QString Q = QString("SELECT * FROM `records` WHERE `key`='%1'").arg(Key);
            QByteArray BA = Q.toLocal8Bit();
            mysql_query(DB, BA.data());
            MYSQL_RES *Result = mysql_store_result(DB);
            unless (Result) db_error();
            MYSQL_ROW Row = mysql_fetch_row(Result);
            if (mysql_num_rows(Result) == 0) {
                QString Q = QString("INSERT INTO `records` VALUES('%1','%2')").arg(Key).arg(QString());
                QByteArray BA = Q.toLocal8Bit();
                if (mysql_query(DB, BA.data())) db_error();
                R->Data = "";
            } else {
                R->Data = Row[2] ? Row[2] : "";
            }
            mysql_free_result(Result);
        }
        R->Next->Prev = R->Prev;
        R->Prev = MostRecentlyUsed;
        MostRecentlyUsed->Next = R;
        MostRecentlyUsed = R;
        MostRecentlyUsed->Next = 0;

        Mutex.unlock();

        R->Mutex.lock();
        return reference(R);
    }

    void flushRecord(record *R) {
        R->Mutex.lock();
        if (R->Modified) {
            QString Q = QString("UPDATE `records` SET `data`='%1' WHERE `key`='%2'").arg(R->Data).arg(R->Key);
            QByteArray BA = Q.toLocal8Bit();
            if(mysql_query(DB, BA.data())) db_error();
            R->Modified = false;
        }
        R->Mutex.unlock();
    }

    // Flushes all records. Could be set on timer
    // Also consider incremental flushing
    void flush() {
        Mutex.lock();
        each(R,Records) {
            flushRecord(R);
            R->Mutex.unlock();
        }
        Mutex.unlock();
    }

private:
    MYSQL *DB;
    QVector<record*> Records;
    record *LeastRecentlyUsed;
    record *MostRecentlyUsed;
    QHash<QString,record*> Hash; //maps keys to fetched records
    QMutex Mutex;
};

int main(int argc, char *argv[]) {
    reference R;
    cache Cache("localhost","root","","rocket_jump",true);
    R = Cache.get("Some Key");
    R.setData("Data Value");
    R.release();
}
