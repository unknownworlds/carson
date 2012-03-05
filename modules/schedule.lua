schedule = { }

local function strtime(time)
	
	local hour, min, am = string.match(time, "(%S+):(%S+)%s+(%S*)")
	local t = os.date("*t")
	t.hour = hour
	t.min  = min
	t.sec  = 0

	-- Convert to a 12 hour clock
	if string.upper(am) == "PM" then
		t.hour = t.hour + 12
	end
	
	return os.time(t)
	
end

-- Sets the exit code based on whether or not the specified time of the day
-- has passed since the last build. Example usage: schedule.daily("12:00 AM")
function schedule.daily(time)

	local currentTime = os.time()
	local runTime     = strtime(time)
	os.exit( currentTime >= runTime and runTime > _LAST_TIME_RUN )
		
end

return schedule