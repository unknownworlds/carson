svn = { }

-- Returns the time of the last commit to the repository. The returned value
-- is compatible with the os.date and os.difftime functions.
-- The path can either specify the URL of a SVN repository or a working copy.
-- In the case where a URL is specified, the last commit to the repository is
-- returned. In the case where a path is specified, the last commit present
-- in the working copy is returned.
function svn.lastcommit(path)

    local result = os.capture("svn info " .. path)
    
    -- Note, svn info should always show information for the OS timezone.
    local year, month, day, hour, min, sec =
        string.match(result, "Last Changed Date: (%S+)-(%S+)-(%S+) (%S+):(%S+):(%S+)")
    return os.time { year=year, month=month, day=day, hour=hour, min=min, sec=sec }

end

function svn.update(path)
    os.execute("svn update " .. path)
end

return svn