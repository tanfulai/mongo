// strategy_simple.cpp

#include "stdafx.h"
#include "request.h"
#include "../client/connpool.h"
#include "../db/commands.h"

namespace mongo {

    class SingleStrategy : public Strategy {

        virtual void queryOp( Request& r ){
            QueryMessage q( r.d() );
            
            bool lateAssert = false;
        
            log(3) << "single query: " << q.ns << "  " << q.query << endl;

            try {
                if ( q.ntoreturn == 1 && strstr(q.ns, ".$cmd") ) {
                    BSONObjBuilder builder;
                    bool ok = runCommandAgainstRegistered(q.ns, q.query, builder);
                    if ( ok ) {
                        BSONObj x = builder.done();
                        replyToQuery(0, r.p(), r.m(), x);
                        return;
                    }
                }
                
                lateAssert = true;
                doQuery( r , r.singleServerName() );
            }
            catch ( AssertionException& e ) {
                assert( !lateAssert );
                BSONObjBuilder err;
                err.append("$err", string("mongos: ") + (e.msg.empty() ? "assertion during query" : e.msg));
                BSONObj errObj = err.done();
                replyToQuery(QueryResult::ResultFlag_ErrSet, r.p() , r.m() , errObj);
                return;
            }

        }
        
        virtual void getMore( Request& r ){
            const char *ns = r.getns();
        
            log(3) << "single getmore: " << ns << endl;

            ScopedDbConnection dbcon( r.singleServerName() );
            DBClientBase& _c = dbcon.conn();

            // TODO 
            DBClientConnection &c = dynamic_cast<DBClientConnection&>(_c);

            Message response;
            bool ok = c.port().call( r.m() , response);
            uassert("dbgrid: getmore: error calling db", ok);
            r.reply( response );
        
            dbcon.done();

        }
        
        virtual void writeOp( int op , Request& r ){
            const char *ns = r.getns();
            log(3) << "single write: " << ns << endl;
            doWrite( op , r , r.singleServerName() );
        }
    };
    
    Strategy * SINGLE = new SingleStrategy();
}
