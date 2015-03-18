<?php
class ChatServer
{
	protected $ip;
	protected $pid;
	protected $host;
	protected $port;
	protected $bLog;
	protected $sFileRegister;
	protected $sFileLog;
	private static $aLogMessages=array(
		1000=>'Server has started successfully',
		1001=>'Server is running',
		1002=>'Server has stopped',
		2000=>'Unknown error',
		2001=>'socket_create() failed: reason',
		2002=>'socket_bind() failed: reason:',
		2003=>'socket_listen() failed: reason:',
		2004=>'socket_accept() failed: reason:',
		2005=>'socket_read() failed: reason:',
		2006=>'socket_select() failed: reason:',
		2007=>'Could not fork process',
		2008=>'Could not create path for pid file',
		2009=>'Could not register server',
		2010=>'Could not register connection'
	);
	public function __construct($sHost,$iPort,$bLog=false)
	{
		$this->host=$sHost;
		$this->port=$iPort;
		$this->ip=getIp();
		$this->pid=posix_getpid();
		$this->bLog=$bLog;
		$sCurDir=dirname(__FILE__);
		$this->sFileRegister=$sCurDir.'/../log/pid/'.$sHost.':'.$iPort.'.pid';
		$this->sFileLog=$sCurDir.'/../log/'.$sHost.':'.$iPort.'.log';
	}
	public function __destruct()
	{
		if($this->pid!==posix_getpid())
			$this->log(1002);
	}
	public function start()
	{
		if($this->isRunning())
		{
			$this->log(1001);
			return false;
		}
		$pid=pcntl_fork();
		if($pid==-1)
		{
			$this->log(2007);
			return false;
		}
		elseif($pid)
			return true;
		else
			$this->register();
		return true;
	}
	public function stop()
	{
		$pid=$this->getPid();
		if(!$pid)
			return false;
		posix_kill($pid,SIGKILL);
		return ChatLogger::writeFile($this->sFileLog,'',true);
	}
	protected function getPid()
	{
		if(!is_file($this->sFileRegister))
			return false;
		$sJSON=file_get_contents($this->sFileRegister);
		$aInfo=json_decode($sJSON);
		return isset($aInfo[0])?$aInfo[0]:false;
	}
	protected function register()
	{
		if(($sock = socket_create(AF_INET, SOCK_STREAM, SOL_TCP)) === false)
		{
			$this->log(2001,socket_strerror(socket_last_error()));
			return false;
		}
		register_shutdown_function('ChatServer::unregister',$sock,$this);
		if(socket_bind($sock, $this->host, $this->port) === false)
		{
			$this->log(2002,socket_strerror(socket_last_error($sock)));
			return false;
		}
		if(socket_listen($sock,5) === false)
		{
			$this->log(2003,socket_strerror(socket_last_error($sock)));
			return false;
		}
		if($this->registerProcess(true))
			$this->log(1000);
		else
		{
			$this->log(2009);
			return false;
		}
		return $this->registerClients($sock);
	}
	protected function registerProcess($bServer=false)
	{
		if(!ChatLogger::mkpath(dirname($this->sFileRegister)))
		{
			$this->log(2008);
			return false;
		}
		$sJSON=json_encode(array(
			posix_getpid(),
			$this->ip,
			date('c',time())
		));
		return ChatLogger::writeFile($this->sFileRegister,$sJSON,$bServer);
	}
	protected function registerClients($sock)
	{
		$aClients=$aTerminals=array();
		do
		{
			$aRead=array_merge(array($sock),$aClients);
			$aWrite=$aExcept=null;
			if(!socket_select($aRead,$aWrite,$aExcept,5))
			{
				$this->log(2006,socket_strerror(socket_last_error($sock)));
				continue;
			}
			if(in_array($sock, $aRead))
			{
				if(($msgsock = socket_accept($sock)) === false)
				{
					$this->log(2004,socket_strerror(socket_last_error($sock)));
					break;
				}
				$oTerminal=new ChatTerminal($msgsock,$this->sFileLog,$this->bLog);
				$aClients[]=$msgsock;
				$aTerminals[]=$oTerminal;
				$iCntLogged=0;
				foreach($aTerminals as $oX)
					if($oX->isClientLogged())
						$iCntLogged++;
				$oTerminal->sayhello($iCntLogged);
			}
			foreach($aClients as $iKey=>$oClient)
			{
				if(!in_array($oClient,$aRead))
					continue;
				if(false === ($buf = socket_read($oClient, 2048, PHP_NORMAL_READ)))
				{
					$this->log(2005,socket_strerror(socket_last_error($oClient)));
					break 2;
				}
				$xStatus=$aTerminals[$iKey]->parsing($buf,$aClients,$aTerminals);
				if($xStatus===ChatTerminal::STATUS_QUIT)
				{
					socket_close($oClient);
					unset($aClients[$iKey],$aTerminals[$iKey]);
					break;
				}
			}
		} while(true);
	}
	public static function unregister($sock,$oServer)
	{
		if($sock)
			socket_close($sock);
		if($oServer instanceof ChatServer)
			unset($oServer);
	}
	public function isRunning()
	{
		$pid=$this->getPid();
		return $pid?(bool)posix_getsid($pid):false;
	}
	public function log($iMsgCode,$sAddMsg='')
	{
		if(!$this->bLog)
			return false;
		$sMessage=isset(self::$aLogMessages[$iMsgCode])?self::$aLogMessages[$iMsgCode]:'';
		$sJSON=json_encode(array(
			$this->pid,
			posix_getpid(),
			$this->ip,
			date('c',time()),
			$iMsgCode,
			implode(' ',array($sMessage,$sAddMsg))
		)).PHP_EOL;
		return ChatLogger::writeFile($this->sFileLog,$sJSON);
	}
}
?>