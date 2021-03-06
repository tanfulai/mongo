/* commands.cpp
   db "commands" (sent via db.$cmd.findOne(...))
 */

/**
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include "query.h"
#include "pdfile.h"
#include "jsobj.h"
#include "../util/builder.h"
#include <time.h>
#include "introspect.h"
#include "btree.h"
#include "../util/lruishmap.h"
#include "javajs.h"
#include "json.h"
#include "repl.h"
#include "commands.h"

namespace mongo {

    const int edebug=0;

    bool dbEval(const char *ns, BSONObj& cmd, BSONObjBuilder& result, string& errmsg) {
        BSONElement e = cmd.firstElement();
        assert( e.type() == Code || e.type() == CodeWScope || e.type() == String );

        const char *code = 0;
        switch ( e.type() ) {
        case String:
        case Code:
            code = e.valuestr();
            break;
        case CodeWScope:
            code = e.codeWScopeCode();
            break;
        default:
            assert(0);
        }
        assert( code );

        if ( ! JavaJS ) {
            errmsg = "db side execution is disabled";
            return false;
        }

#if !defined(NOJNI)
        jlong f = JavaJS->functionCreate(code);
        if ( f == 0 ) {
            errmsg = "compile failed";
            return false;
        }

        Scope s;
        if ( e.type() == CodeWScope )
            s.init( e.codeWScopeScopeData() );
        s.setString("$client", database->name.c_str());
        BSONElement args = cmd.findElement("args");
        if ( args.type() == Array ) {
            BSONObj eo = args.embeddedObject();
            if ( edebug ) {
                out() << "args:" << eo.toString() << endl;
                out() << "code:\n" << code << endl;
            }
            s.setObject("args", eo);
        }

        int res;
        {
            Timer t;
            res = s.invoke(f);
            int m = t.millis();
            if ( m > 100 ) {
                out() << "dbeval slow, time: " << dec << m << "ms " << ns << endl;
                OCCASIONALLY log() << code << endl;
                else if ( m >= 1000 ) log() << code << endl;
            }
        }
        if ( res ) {
            result.append("errno", (double) res);
            errmsg = "invoke failed: ";
            errmsg += s.getString( "error" );
            return false;
        }

        int type = s.type("return");
        if ( type == Object || type == Array )
            result.append("retval", s.getObject("return"));
        else if ( type == NumberDouble )
            result.append("retval", s.getNumber("return"));
        else if ( type == String )
            result.append("retval", s.getString("return").c_str());
        else if ( type == Bool ) {
            result.appendBool("retval", s.getBoolean("return"));
        }
#endif
        return true;
    }

    class CmdEval : public Command {
    public:
        virtual bool slaveOk() {
            return false;
        }
        CmdEval() : Command("$eval") { }
        bool run(const char *ns, BSONObj& cmdObj, string& errmsg, BSONObjBuilder& result, bool fromRepl) {
            return dbEval(ns, cmdObj, result, errmsg);
        }
    } cmdeval;

} // namespace mongo
