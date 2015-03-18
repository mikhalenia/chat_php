<?php
class ChatTerminal
{
	const STATUS_QUIT='quit';
	const STATUS_TYPE_PASSWD='passwd';
	protected $oAuth;
	protected $oSock;
	protected $sFileLog;
	protected $bLog;
	protected $ip;
	protected $pid;
	private static $aMessages=array(
		1000=>"\nWelcome to the Chat! Now, in this room are %s users.\nTo quit, type '/quit'.\nTo login, type '/login'.\nTo logout, type '/logout'.\nTo register, type '/register'.\n",
		1001=>"Good bye!\n",
		1002=>"You have to login.\n",
		1003=>"You have to type your name.\n",
		1004=>"That Login does not exist.\n",
		1005=>"Invalid password.\n",
		1006=>"That login is busy.\n",
		1007=>"Passwords don't match.\n",
		1008=>"Enter the password.\n",
		1009=>"Repeat your password.\n",
		1010=>"Invalid login.\n"
	);
	private static $aLogMessages=array(
		1000=>"Client has connected"
	);
	public function __construct($oSock,$sFileLog,$bLog)
	{
		$this->ip=getIp();
		$this->pid=posix_getpid();
		$this->bLog=$bLog;
		$this->oSock=$oSock;
		$this->sFileLog=$sFileLog;
		$this->oAuth=new ChatAuth($sFileLog,$bLog);
	}
	public function sayhello($iCntUsers)
	{
		$this->writeMsg(1000,$iCntUsers);
	}
	public function parsing($buf,&$aClients,&$aTerminals)
	{
		if(!$buf=trim($buf))
			return false;
		$aBuf=explode(' ',$buf);
		$bIsCreate=$this->oAuth->isCreateLogin();
		if($this->oAuth->isTryLogin())
			$sCmd='/passwd';
		elseif($bIsCreate)
			$sCmd='/passwd';
		elseif($this->oAuth->isTryPasswd())
			$sCmd='/checkpasswd';
		else
			$sCmd=array_shift($aBuf);
		switch($sCmd)
		{
			case '/quit':
				$this->writeMsg(1001);
				return self::STATUS_QUIT;
			case '/login':
				$sLogin=array_shift($aBuf);
				if(!$sLogin)
				{
					$this->writeMsg(1003);
					return false;
				}
				if(!preg_match('/^\pL{1}[\pL|\pN]{1,10}$/ui',$sLogin))
				{
					$this->writeMsg(1010);
					return false;
				}
				if(!$this->oAuth->setLogin($sLogin))
				{
					$this->writeMsg(1004);
					return false;
				}
				$this->writeMsg(1008);
				return self::STATUS_TYPE_PASSWD;
			case '/passwd':
				if(!$this->oAuth->setPasswd(array_shift($aBuf)))
				{
					$this->writeMsg(1005);
					return false;
				}
				elseif($bIsCreate)
				{
					$this->writeMsg(1009);
					return self::STATUS_TYPE_PASSWD;
				}
				else
					$sTalkback=$this->oAuth->getName().">Joined\n";
				break;
			case '/checkpasswd':
				if(!$this->oAuth->checkPasswd(array_shift($aBuf)))
				{
					$this->writeMsg(1007);
					return false;
				}
				$sTalkback=$this->oAuth->getName().">Joined\n";
				break;
			case '/register':
				$sLogin=array_shift($aBuf);
				if(!$sLogin)
				{
					$this->writeMsg(1003);
					return false;
				}
				if(!preg_match('/^\pL{1}[\pL|\pN]{1,10}$/ui',$sLogin))
				{
					$this->writeMsg(1010);
					return false;
				}
				if(!$this->oAuth->registerLogin($sLogin))
				{
					$this->writeMsg(1006);
					return false;
				}
				$this->writeMsg(1008);
				return self::STATUS_TYPE_PASSWD;
			case '/logout':
				if(!$this->oAuth->isLogged())
				{
					$this->writeMsg(1002);
					return false;
				}
				$sTalkback=$this->oAuth->getName().">Left\n";
				$this->oAuth->logout();
				break;
			default:
				if(!$this->oAuth->isLogged())
				{
					$this->writeMsg(1002);
					return false;
				}
				$sTalkback=$this->oAuth->getName().">{$buf}\n";
				break;
		}
		$iLenMsg=strlen($sTalkback);
		socket_write($this->oSock,$sTalkback,$iLenMsg);
		foreach($aClients as $iKey=>$oClientOth)
		{
			if(
				$this->oSock===$oClientOth||
				!$aTerminals[$iKey]->isClientLogged()
			)
				continue;
			socket_write($oClientOth,$sTalkback,$iLenMsg);
		}
		return true;
	}
	public function isClientLogged()
	{
		return $this->oAuth->isLogged();
	}
	protected function writeMsg($iMsgCode,$xAddInfo='')
	{
		$sMsg=isset(self::$aMessages[$iMsgCode])?sprintf(self::$aMessages[$iMsgCode],$xAddInfo):'';
		socket_write($this->oSock,$sMsg,strlen($sMsg));
	}
	protected function log($iMsgCode,$sAddMsg='')
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