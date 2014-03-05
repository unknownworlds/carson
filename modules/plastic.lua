plastic = { }

-- Returns the time of the last commit to the repository. The returned value
-- is compatible with the os.date and os.difftime functions.
-- The path specifies the working copy.
function plastic.lastcommit(path)

    local result = os.capture("cd /d " .. path .. " && cm describebranchhistory main")
	
    local lastChange = { day = nil, month = nil, year = nil, hour = nil, min = nil, sec = nil }
    for month, day, year, hour, min, sec, ampm in string.gmatch(result, "Date: (%S+)/(%S+)/(%S+) (%S+):(%S+):(%S+) (%S+)") do

		lastChange.day = day
        lastChange.month = month
        lastChange.year = year
        lastChange.hour = tostring(tonumber(hour) + ((ampm == "PM") and 12 or 0))
        lastChange.min = min
        lastChange.sec = sec
        
    end

	return os.time(lastChange)

end

function plastic.update(path)
    os.execute("cm update " .. path)
end

return plastic