/***************************************************************************
 *   Copyright (C) 2006 by Ricky White                                     *
 *   rickyw@sourceforge.net                                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <cstdlib>
#include <list>
#include <signal.h>
#include <getopt.h>
#include <iostream>
#include <fstream>
#include <readline/readline.h>
#include <readline/history.h>

#include "cdbfile.h"
#include "parsecmd.h"
#include "cmdcommon.h"
#include "cmdbreakpoints.h"
#include "cmddisassemble.h"
#include "cmdmaintenance.h"
#include "targetsilabs.h"
#include "targets51.h"
#include "newcdb.h"

#ifdef _SYS_SIGNAL_H_ 
/* This appears to be a BSD variant which also uses a different type name */ 
typedef sig_t sighandler_t; 
#endif




using namespace std;

ParseCmd::List cmdlist;
string prompt;

DbgSession gSession;

void sig_int_handler(int)
{
	gSession.target()->stop();
	cout << endl << prompt;
}

void quit()
{
	gSession.target()->stop();
	gSession.target()->disconnect();
}


bool parse_cmd( string ln )
{
	if( ln.compare("quit")==0 )
	{
		gSession.target()->disconnect();
		exit(0);
	}
	ParseCmd::List::iterator it;
	for( it=cmdlist.begin(); it!=cmdlist.end(); ++it)
	{
		if( (*it)->parse(ln) )
			return true;
	}
	return ln.length()==0;	// anything left with length >0 is bad.
}

bool process_cmd_file( string filename )
{
	string line;
	ifstream cmdlist( filename.c_str() );
	if( !cmdlist.is_open() )
		return false;	// failed to open command list file

	while( !cmdlist.eof() )
	{
		std::getline(cmdlist, line);
		cout << line << endl;
		if( !parse_cmd(line) )
			cout << "Bad Command" << endl;
	}
	cmdlist.close();
	return true;
}



int main(int argc, char *argv[])
{
	sighandler_t	old_sig_int_handler;

	old_sig_int_handler = signal( SIGINT, sig_int_handler );
	atexit(quit);
//	target = new TargetS51();
//	target->connect();

	CdbFile f(&gSession);

	// add commands to list
	cmdlist.push_back( new CmdShowSetInfoHelp() );
	cmdlist.push_back( new CmdVersion() );
	cmdlist.push_back( new CmdWarranty() );
	cmdlist.push_back( new CmdCopying() );
	cmdlist.push_back( new CmdHelp() );
	cmdlist.push_back( new CmdPrompt() );
	cmdlist.push_back( new CmdBreakpoints() );
	cmdlist.push_back( new CmdBreak() );
	cmdlist.push_back( new CmdTBreak() );
	cmdlist.push_back( new CmdDelete() );
	cmdlist.push_back( new CmdEnable() );
	cmdlist.push_back( new CmdDisable() );
	cmdlist.push_back( new CmdClear() );
	cmdlist.push_back( new CmdTarget() );
	cmdlist.push_back( new CmdStep() );
	cmdlist.push_back( new CmdStepi() );
	cmdlist.push_back( new CmdNext() );
	cmdlist.push_back( new CmdNexti() );
	cmdlist.push_back( new CmdContinue() );
	cmdlist.push_back( new CmdFile() );
	cmdlist.push_back( new CmdFiles() );
	cmdlist.push_back( new CmdList() );
	cmdlist.push_back( new CmdPWD() );
	cmdlist.push_back( new CmdSource() );
	cmdlist.push_back( new CmdSources() );
	cmdlist.push_back( new CmdLine() );
	cmdlist.push_back( new CmdRun() );
	cmdlist.push_back( new CmdStop() );
	cmdlist.push_back( new CmdFinish() );
	cmdlist.push_back( new CmdDisassemble() );
	cmdlist.push_back( new CmdX() );
	cmdlist.push_back( new CmdChange() );
	cmdlist.push_back( new CmdMaintenance() );
	cmdlist.push_back( new CmdPrint() );
	cmdlist.push_back( new CmdRegisters() );
	string ln;
	prompt = "(newcdb) ";
	FILE *badcmd = 0;
	int quiet_flag = 0;
	int help_flag = 0;
	int debug_badcmd_flag = 0;
	int fullname_flag = 0;
	while (1)
	{
		// command line option parsing
		static struct option long_options[] =
		{
			{"command", required_argument, 0, 'c'},
			{"ex", required_argument, 0, 'e'},
			{"dbg-badcmd", required_argument, 0, 'b'},
			{"fullname", no_argument, &fullname_flag, 1},
			{"q", no_argument, &quiet_flag, 1},
			{"help", no_argument, &help_flag, 1},
			{0, 0, 0, 0}
		};

		/* getopt_long stores the option index here. */
		int option_index = 0;
#ifdef __GLIBC__
		int c = getopt_long_only( argc, argv, "", long_options, &option_index);
#else
		int c = getopt_long(argc, argv, "", long_options, &option_index);
#endif
		/* Detect the end of the options. */
		if( c == -1 )
			break;

		switch (c)
		{
			case 0:
				/* If this option set a flag, do nothing else now. */
				if (long_options[option_index].flag != 0)
					break;
				printf ("option %s", long_options[option_index].name);
				if (optarg)
					printf (" with arg %s", optarg);
				printf ("\n");
				break;
			case 'b':
				if(!badcmd)
				{
					badcmd = fopen(optarg,"w");
					if(!badcmd)
					{
						cerr << "ERROR: Failed to open "<<optarg
							 << "for bad command logging."<<endl;
					}
				}	
				break;
			case 'c':
				// Command file
				cout << "Processing command file '" << optarg << "'" << endl;
				if( process_cmd_file(optarg) )
					cout << "ERROR coulden't open command file" << endl;
				break;
			case 'e':
				// Command file
				cout << "Executing command " << optarg << endl;
				add_history(optarg);
				if( !parse_cmd(optarg) )
					printf("Bad Command.");
				break;
			default:
				abort();
		}
	}
	/* Print any remaining command line arguments (not options). */
	if( optind < argc )
	{
//		printf ("non-option ARGV-elements: ");
//		while (optind < argc)
//			printf ("%s ", argv[optind++]);
//		putchar ('\n');
		while (optind < argc)
			parse_cmd(string("file ") + argv[optind++] );
	}

	if( help_flag )
	{
		cout << "newcdb, new ec2cdb based on c++ source code" << endl
			<< "Help:" << endl
			<< "\t-command=<file>   Execute the commands listed in the supplied\n" 
			<< "\t                  file in ordser top to bottom.\n"
			<< "\t-ex=<command>     Execute the command as if it were typed on \n"
			<< "\t                  the command line of newcdb once newcdb\n"
			<< "\t                  starts.  You can have multiple -ex options\n"
			<< "\t                  and they will be executed in order left to\n"
			<< "\t                  right.\n"
			<< "\t-fullname         Sets the line number output format to two `\\032'\n"
			<< "\t                  characters, followed by the file name, line number\n"
			<< "\t                  and character position separated by colons, and a newline.\n"
			<< "\t-q                Suppress the startup banner\n"
			<< "\t--dbg-badcmd=file Log all bad commands to file\n"
			<< "\t--help            Display this help"
			<< endl << endl;
		exit(0);
	}

	if( !quiet_flag )
	{
		cout << "newcdb, new ec2cdb based on c++ source code" << endl;
	}


	while(1)
	{
		bool ok=false;
		char *line = readline( prompt.c_str() );
		if(*line!=0)
			add_history(line);
		ln = line;
		free(line);
		if(badcmd)
			fwrite((ln+'\n').c_str(),1,ln.length()+1, badcmd);
		if( ln.compare("quit")==0 )
		{
			signal( SIGINT, old_sig_int_handler );
			gSession.target()->disconnect();
			if(badcmd)
				fclose(badcmd);
			return 0;
		}
		ParseCmd::List::iterator it;
		for( it=cmdlist.begin(); it!=cmdlist.end(); ++it)
		{
			if( (*it)->parse(ln) )
			{
				ok = true;
				break;
			}
		}
		if( !ok && (ln.length()>0))
		{
			cout <<"bad command ["<<ln<<"]"<<endl;
			if( badcmd!=0 )
			{
				fwrite(("BAD: "+ln+'\n').c_str(),1,ln.length()+1, badcmd);
				fflush(badcmd);
			}
		}

	}
	fclose(badcmd);
	return EXIT_SUCCESS;
}


