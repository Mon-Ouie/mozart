###
### Authors:
###   Denys Duchier <duchier@ps.uni-sb.de>
###   Christian Schulte <schulte@dfki.de>
###
### Copyright:
###   Denys Duchier, 1998
###   Christian Schulte, 1998
###
### Last change:
###   $Date$ by $Author$
###   $Revision$
###
### This file is part of Mozart, an implementation 
### of Oz 3:
###    http://mozart.ps.uni-sb.de
###
### See the file "LICENSE" or
###    http://mozart.ps.uni-sb.de/LICENSE.html
### for information on usage and redistribution 
### of this file, and for a DISCLAIMER OF ALL 
### WARRANTIES.
###

%builtins_all =
(
    'getDir'		=> { in  => ['+virtualString'],
			     out => ['+[string]'],
			     BI  => unix_getDir},

    'stat'		=> { in  => ['+virtualString'],
			     out => ['+record'],
			     BI  => unix_stat},

    'chDir'		=> { in  => ['+virtualString'],
			     out => [],
			     BI  => unix_chDir},

    'getCWD'		=> { in  => [],
			     out => ['+atom'],
			     BI  => unix_getCWD},

    'open'		=> { in  => ['+virtualString','+[atom]','+[atom]'],
			     out => ['+int'],
			     BI  => unix_open},

    'fileDesc'	=> { in  => ['+atom'],
			     out => ['+int'],
			     BI  => unix_fileDesc},

    'close'		=> { in  => ['+int'],
			     out => [],
			     BI  => unix_close},

    'write'		=> { in  => ['+int','+virtualString'],
			     out => ['+value'],
			     BI  => unix_write},

    'read'		=> { in  => ['+int','+int','value','value','int'],
			     out => [],
			     BI  => unix_read},

    'lSeek'		=> { in  => ['+int','+int','+atom'],
			     out => ['+int'],
			     BI  => unix_lSeek},

    'unlink'		=> { in  => ['+virtualString'],
			     out => [],
			     BI  => unix_unlink},

    'readSelect'	=> { in  => ['+int'],
			     out => [],
			     BI  => unix_readSelect},

    'writeSelect'	=> { in  => ['+int'],
			     out => [],
			     BI  => unix_writeSelect},

    'acceptSelect'	=> { in  => ['+int'],
			     out => [],
			     BI  => unix_acceptSelect},

    'deSelect'	=> { in  => ['+int'],
			     out => [],
			     BI  => unix_deSelect},

    'system'		=> { in  => ['+virtualString'],
			     out => ['+int'],
			     BI  => unix_system},

    'getEnv'		=> { in  => ['+virtualString'],
			     out => ['+value'],
			     BI  => unix_getEnv},

    'putEnv'		=> { in  => ['+virtualString','+virtualString'],
			     out => [],
			     BI  => unix_putEnv},

    'time'		=> { in  => [],
			     out => ['+int'],
			     BI  => unix_time},

    'gmTime'		=> { in  => [],
			     out => ['+record'],
			     BI  => unix_gmTime},

    'localTime'	=> { in  => [],
			     out => ['+record'],
			     BI  => unix_localTime},

    'srand'		=> { in  => ['+int'],
			     out => [],
			     BI  => unix_srand},

    'rand'		=> { in  => [],
			     out => ['+int'],
			     BI  => unix_rand},

    'randLimits'	=> { in  => [],
			     out => ['+int','+int'],
			     BI  => unix_randLimits},

    'socket'		=> { in  => ['+atom','+atom','+virtualString'],
			     out => ['+int'],
			     BI  => unix_socket},

    'bind'		=> { in  => ['+int','+int'],
			     out => [],
			     BI  => unix_bindInet},

    'listen'		=> { in  => ['+int','+int'],
			     out => [],
			     BI  => unix_listen},

    'connect'	=> { in  => ['+int','+virtualString','+int'],
			     out => [],
			     BI  => unix_connectInet},

    'accept'		=> { in  => ['+int'],
			     out => ['+int','+string','+int'],
			     BI  => unix_acceptInet},

    'shutDown'	=> { in  => ['+int','+int'],
			     out => [],
			     BI  => unix_shutDown},

    'send'		=> { in  => ['+int','+virtualString','+[atom]'],
			     out => ['+value'],
			     BI  => unix_send},

    'sendTo'		=> { in  => ['+int','+virtualString','+[atom]',
				     '+virtualString','+int'],
			     out => ['+value'],
			     BI  => unix_sendToInet},

    'receiveFrom'	=> { in  => ['+int','+int','+[atom]','value','value'],
			     out => ['+string','+int','+int'],
			     BI  => unix_receiveFromInet},

    'getSockName'	=> { in  => ['+int'],
			     out => ['+int'],
			     BI  => unix_getSockName},

    'getHostByName'	=> { in  => ['+virtualString'],
			     out => ['+record'],
			     BI  => unix_getHostByName},

    'pipe'		=> { in  => ['+virtualString','value'],
			     out => ['+int','+int#int'],
			     BI  => unix_pipe},

    'exec'		=> { in  => ['+virtualString','value'],
			     out => ['+int'],
			     BI  => unix_exec},

    'tmpnam'		=> { in  => [],
			     out => ['+string'],
			     BI  => unix_tmpnam},

    'wait'		=> { in  => [],
			     out => ['+int','+int'],
			     BI  => unix_wait},

    'getServByName'	=> { in  => ['+virtualString','+virtualString'],
			     out => ['+int'],
			     BI  => unix_getServByName},

    'uName'		=> { in  => [],
			     out => ['+record'],
			     BI  => unix_uName},

    'getpwnam'	=> { in  => ['+virtualString'],
			     out => ['+record'],
			     BI  => unix_getpwnam},
    'signal'	=> { in  => ['+int','+procedure'],
			     out => [],
			     BI  => unix_signalHandler},

 );
