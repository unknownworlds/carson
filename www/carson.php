<?php

require_once("database.php");

class Project
{
    var $id;
    var $name;
    var $command;
    var $state;
    var $time; // time the project was last built.
}

;

// Name of the database. Can be changed if there are conflicts.
define('DB_NAME', 'carson');
define('CONFIG_FILENAME', '../carson.config');

function carson_install($dbHost, $dbUserName, $dbPassword)
{

    if (!db_connect($dbHost, $dbUserName, $dbPassword)) {
        die("Couldn't connect to database");
    }

    // Write the config file.
    $config = "DB_HOST     = '$dbHost'\n";
    $config .= "DB_USERNAME = '$dbUserName'\n";
    $config .= "DB_PASSWORD = '$dbPassword'";
    file_put_contents(CONFIG_FILENAME, $config);

    // Setup the database.
    db_exec("DROP DATABASE IF EXISTS " . DB_NAME);
    db_exec("CREATE DATABASE " . DB_NAME);
    db_select(DB_NAME);
    db_exec("CREATE TABLE projects (id INT NOT NULL AUTO_INCREMENT, PRIMARY KEY(id), name TINYTEXT, test MEDIUMTEXT, command MEDIUMTEXT, enabled BOOLEAN)");
    db_exec("CREATE TABLE project_builds (id INT NOT NULL AUTO_INCREMENT, PRIMARY KEY(id), projectId INT, state TINYTEXT, time DATETIME, log LONGTEXT)");

}

function carson_installed()
{
    return file_exists(CONFIG_FILENAME);
}

/** Loads the configuration and connects to the database. */
function carson_connect()
{
    $lines = file(CONFIG_FILENAME);
    $config = array();
    foreach ($lines as $line) {
        if (preg_match("/^\s*(\S*)\s*=\s*\'(\S*)\'$/", $line, $matches)) {
            $config[$matches[1]] = $matches[2];
        }
    }

    if (!isset($config['DB_HOST']) ||
        !isset($config['DB_USERNAME']) ||
        !isset($config['DB_PASSWORD'])
    ) {
        die("Config is invalid");
    }

    $dbHost = $config['DB_HOST'];
    $dbUserName = $config['DB_USERNAME'];
    $dbPassword = $config['DB_PASSWORD'];

    if (!db_connect($dbHost, $dbUserName, $dbPassword) or !db_select(DB_NAME)) {
        die("Couldn't connect to database");
    }

}

/** Adds a new new project to the system. */
function carson_addProject($project)
{
    $name = db_escape($project->name);
    $command = db_escape($project->command);
    $trigger = db_escape($project->trigger);
    db_exec("INSERT INTO projects (name, command, test, enabled) VALUES ('$name', '$command', '$trigger', TRUE)");
    $project->id = db_getLastInsertId();
    db_exec("INSERT INTO project_builds (projectId, state, time, log) VALUES ('{$project->id}', 'new', NOW(), '')");
}

function carson_updateProject($project)
{
    $id = db_escape($project->id);
    $name = db_escape($project->name);
    $command = db_escape($project->command);
    $trigger = db_escape($project->trigger);
    db_exec("UPDATE projects SET name='$name', command='$command', test='$trigger' WHERE id='$id'");
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
    foreach ($rows as $row) {
        $project = new Project();
        $project->id = $row['id'];
        $project->name = $row['name'];
        $project->command = $row['command'];
        $project->trigger = $row['test'];
        $project->enabled = $row['enabled'];
        $projects[] = $project;
    }

    // Get the information about the builds.
    $rows = db_query("SELECT * FROM project_builds");
    foreach ($rows as $row) {
        foreach ($projects as $project) {
            if ($project->id == $row['projectId']) {
                $project->state = $row['state'];
                $project->time = strtotime($row['time']);
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

function carson_enableProject($id, $enabled)
{
    $id = db_escape($id);
    $enabled = $enabled ? "true" : "false";
    db_exec("UPDATE projects SET enabled=$enabled WHERE id='$id'");
}

function carson_getProjectLog($id, $openedEvents = false)
{
    $id = db_escape($id);
    $rows = db_query("SELECT * FROM project_builds WHERE projectId='$id'");
    if (count($rows) > 0) {
        $input = trim($rows[0]['log']);

        if (substr($input, -6) !== '</div>' && !empty($input)) {
            $input .= '</pre><div class="status">Working</div></div>';
        }

        $dom = new DOMDocument();

        if (!empty($input)) {
            $loadStatus = $dom->loadHTML($input);
        } else {
            return 'Log is empty.';
        }

        if ($loadStatus) {
            $domXpath = new DOMXpath($dom);

            $elements = $domXpath->query("//div[@class='element']/div[@class='status']");

            if (!is_null($elements)) {
                foreach ($elements as $element) {
                    $nodes = $element->childNodes;
                    foreach ($nodes as $node) {
                        $element->parentNode->setAttribute('class', $element->parentNode->getAttribute('class') . ' ' . $node->nodeValue);
                    }
                }
            }

            if (!empty($openedEvents)) {
                if (!empty($openedEvents[0])) {
                    foreach ($openedEvents as $eventIndex) {
                        $elements = $domXpath->query("(//div[contains(concat(' ',normalize-space(@class),' '),' element ')])[$eventIndex]/pre");
                        if (!is_null($elements)) {
                            foreach ($elements as $element) {
                                //echo 'x';
                                $element->setAttribute('class', 'outputVisible');
                            }
                        }
                    }
                }
            }

            return '<div id="logContainer">'.$dom->saveHTML().'</div>';
        } else {
            return 'Invalid output HTML';
        }
    }
}

?>