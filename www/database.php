<?php

$db = NULL;

function db_connect($host, $user, $password)
{
	global $db;
	$db = mysql_connect($host, $user, $password, true);
	return $db != NULL;
}

function db_select($name)
{
	global $db;
	return mysql_select_db($name, $db);
}

function db_query($query)
{
	global $db;
	$rows = array();
	$result = mysql_query($query, $db);
	if ($result)
	{
		while ($row = mysql_fetch_array($result,  MYSQL_ASSOC))
		{
			$rows[] = $row;
		}
		mysql_free_result($result);	
	}
	return $rows;
}

function db_exec($query)
{
	global $db;
	if (!mysql_query($query, $db))
	{
		echo mysql_error($db);
		return false;
	}
	return true;
}

function db_escape($value)
{
	global $db;
	return mysql_real_escape_string($value, $db);
}

function db_getLastInsertId()
{
	global $db;
	return mysql_insert_id ($db);
}

?>