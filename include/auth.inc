<?php
class ChatAuth
{
	protected $bIsTryLogin;
	protected $bIsTryRegister;
	protected $bIsTryPasswd;
	protected $bIsLogged;
	protected $sFileLog;
	protected $bLog;
	protected $ip;
	protected $pid;
	private $sLogin;
	public function __construct($sFileLog,$bLog)
	{
		$this->ip=getIp();
		$this->pid=posix_getpid();
		$this->bLog=$bLog;
		$this->sFileLog=$sFileLog;
		$this->bIsLogged=false;
		$this->clean();
	}
	private static $aLogMessages=array(
		
	);
	public function getName()
	{
		return $this->bIsLogged?$this->sLogin:false;
	}
	public function isCreateLogin()
	{
		return $this->bIsTryRegister;
	}
	public function isTryLogin()
	{
		return $this->bIsTryLogin;
	}
	public function isTryPasswd()
	{
		return $this->bIsTryPasswd;
	}
	public function setLogin($sLogin)
	{
		if(!preg_match('/^\pL{1}[\pL|\pN]{1,10}$/ui',$sLogin))
			return false;
		$dbh=getDbHandle("utf8");
		$aLogin=$dbh->query("
			SELECT 1
			FROM chat_users
			WHERE login=".$dbh->quote($sLogin)
		)->fetch(PDO::FETCH_ASSOC);
		$this->clean();
		if(!$aLogin)
			return false;
		$this->sLogin=$sLogin;
		$this->bIsTryLogin=true;
		return true;
	}
	public function setPasswd($sPasswd)
	{
		if(!(
			$sPasswd&&
			($this->bIsTryLogin||$this->bIsTryRegister)&&
			$this->sLogin
		))
			return false;
		$dbh=getDbHandle("utf8");
		$sHash=$dbh->quote(md5($sPasswd));
		$sLogin=$this->sLogin;
		if($this->bIsTryLogin)
		{
			$aUser=$dbh->query("
				SELECT 1
				FROM chat_users
				WHERE login=".$dbh->quote($sLogin)." AND
					passwd_left=CONV(LEFT({$sHash},16),16,10) AND
					passwd_right=CONV(RIGHT({$sHash},16),16,10)
			")->fetch(PDO::FETCH_ASSOC);
			$this->clean();
			if(!$aUser)
				return false;
			$this->sLogin=$sLogin;
			$this->bIsLogged=true;
			return true;
		}
		$iRes=$dbh->exec("
			UPDATE chat_users
			SET passwd_left=CONV(LEFT({$sHash},16),16,10), passwd_right=CONV(RIGHT({$sHash},16),16,10)
			WHERE login=".$dbh->quote($sLogin)
		);
		$this->clean();
		if(!$iRes)
			return false;
		$this->sLogin=$sLogin;
		$this->bIsTryPasswd=true;
		return true;
	}
	public function checkPasswd($sPasswd)
	{
		if(!(
			$sPasswd&&
			$this->bIsTryPasswd&&
			$this->sLogin
		))
			return false;
		$dbh=getDbHandle("utf8");
		$sHash=$dbh->quote(md5($sPasswd));
		$sLogin=$this->sLogin;
		$this->clean();

		$aUser=$dbh->query("
			SELECT 1
			FROM chat_users
			WHERE login=".$dbh->quote($sLogin)." AND
				passwd_left=CONV(LEFT({$sHash},16),16,10) AND
				passwd_right=CONV(RIGHT({$sHash},16),16,10)
		")->fetch(PDO::FETCH_ASSOC);
		if(!$aUser)
			return false;
		$this->sLogin=$sLogin;
		$this->bIsLogged=true;
		return true;
	}
	public function registerLogin($sLogin)
	{
		if(!preg_match('/^\pL{1}[\pL|\pN]{1,10}$/ui',$sLogin))
			return false;
		$dbh=getDbHandle("utf8");
		$iRes=$dbh->exec("
			INSERT IGNORE INTO chat_users
			SET login=".$dbh->quote($sLogin)
		);
		$this->clean();
		if(!$iRes)
			return false;
		$this->sLogin=$sLogin;
		$this->bIsTryRegister=true;
		return true;
	}
	public function logout()
	{
		$this->clean();
		$this->bIsLogged=false;
	}
	public function isLogged()
	{
		return $this->bIsLogged;
	}
	protected function clean()
	{
		$this->bIsTryLogin=false;
		$this->bIsTryRegister=false;
		$this->bIsTryPasswd=false;
		$this->sLogin=null;
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