robocopy = { }

local function DoCopy(src, dest, patterns, argString)

	local result = 0
    for p = 1, #patterns do

        local _, _, copyResult = os.execute("robocopy " .. src .. " " .. dest .. " " .. patterns[p] .. " " .. argString)
        if copyResult > result then
            result = copyResult
        end

    end

    return result <= 8
	
end

function robocopy.copy(src, dest, patterns)
    return DoCopy(src, dest, patterns, "/e")
end

function robocopy.mirror(src, dest, patterns)
    return DoCopy(src, dest, patterns, "/mir")
end