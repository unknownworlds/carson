<?php

require_once("database.php");

class Project
{
	var $id;
	var $name;
	var $command;
	var $state;
	var $time;	// time the project was last built.
};

define("DB_NAME", "../carson.db");

db_connect(DB_NAME) or die("Couldn't connect to database");

/** Adds a new new project to the system. */
function carson_addProject($project)
{
	$name    = db_escape($project->name);
	$command = db_escape($project->command);
	$trigger = db_escape($project->trigger);
	db_exec("INSERT INTO projects (name, command, trigger) VALUES ('$name', '$command', '$trigger')");
	$project->id = db_getLastInsertId();
	db_exec("INSERT INTO project_builds (projectId, state, time, log) VALUES ('{$project->id}', 'new', date('now'), '')");
}

function carson_updateProject($project)
{
	$id      = db_escape($project->id);
	$name    = db_escape($project->name);
	$command = db_escape($project->command);
	$trigger = db_escape($project->trigger);
	db_exec("UPDATE projects SET name='$name', command='$command', trigger='$trigger' WHERE id='$id'");
}

function carson_deleteProject($id)
{
	$id = db_escape($id);
	db_exec("DELETE FROM projects WHERE id='$id'");
	db_exec("DELETE FROM project_builds WHERE projectId='$id'");
}

/** Returns an array of all of the projects. */
function carson_getProjects()
{
	$projects = Array();
	$rows = db_query("SELECT * FROM projects");
	foreach ($rows as $row)
	{
		$project = new Project();
		$project->id      = $row['id'];
		$project->name    = $row['name'];
		$project->command = $row['command'];
		$project->trigger = $row['trigger'];
		$projects[] = $project;
	}
	
	// Get the information about the builds.
	$rows = db_query("SELECT * FROM project_builds");
	foreach ($rows as $row)
	{
		foreach ($projects as $project)
		{
			if ($project->id == $row['projectId'])
			{
				$project->state = $row['state'];
				$project->time  = strtotime($row['time']);
			}
		}
	}
	
	return $projects;
}

/** Requests that the project with the specified id be built. */
function carson_buildProject($id)
{
	$id = db_escape($id);
	db_exec("UPDATE project_builds SET state='pending', log='' WHERE projectId='$id' AND state!='building'");
}

function carson_getProjectLog($id)
{
	$id = db_escape($id);
	$rows = db_query("SELECT * FROM project_builds WHERE projectId='$id'");
	if (count($rows) > 0)
	{
		return $rows[0]['log'];
	}
}

?>