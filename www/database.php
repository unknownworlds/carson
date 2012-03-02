<?php

$db = NULL;

function db_connect($name)
{
	global $db;
	$db = new SQLite3($name);
	return $db != NULL;	
}
function db_query($query)
{
	global $db;
	$rows = array();
	$result = @$db->query($query); 
	if ($result)
	{
		while ($row = $result->fetchArray(SQLITE3_ASSOC))
		{ 
			$rows[] = $row;
		} 	
		$result->finalize();
	}
	return $rows;
}

function db_exec($query)
{
	global $db;
	if (!$db->exec($query))
	{
		echo $db->lastErrorMsg();
		return false;
	}
	return true;
}

function db_escape($value)
{
	global $db;
	return $db->escapeString($value);
}

function db_getLastInsertId()
{
	global $db;
	return $db->lastInsertRowID();
}

?>