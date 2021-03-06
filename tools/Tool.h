// Tool.h

#pragma once

#include <string>

#include <boost/program_options.hpp>

#if defined(_WIN32)
#include <io.h>
#endif

#include "client/dbclient.h"

using std::string;

namespace mongo {
    
    class Tool {
    public:
        Tool( string name , string defaultDB="test" , string defaultCollection="");
        virtual ~Tool();

        int main( int argc , char ** argv );
        
        boost::program_options::options_description_easy_init add_options(){
            return _options->add_options();
        }
        void addPositionArg( const char * name , int pos ){
            _positonalOptions.add( name , pos );
        }
        
        string getParam( string name , string def="" ){
            if ( _params.count( name ) )
                return _params[name.c_str()].as<string>();
            return def;
        }
        bool hasParam( string name ){
            return _params.count( name );
        }

        string getNS(){
            if ( _coll.size() == 0 ){
                cerr << "no collection specified!" << endl;
                throw -1;
            }
            return _db + "." + _coll;
        }

        virtual int run() = 0;

        virtual void printExtraHelp( ostream & out );
        
    protected:
        string _name;
        mongo::DBClientConnection _conn;

        string _db;
        string _coll;

    private:
        boost::program_options::options_description * _options;
        boost::program_options::positional_options_description _positonalOptions;

        boost::program_options::variables_map _params;
        
    };
    
}
