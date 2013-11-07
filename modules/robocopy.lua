robocopy = { }

function robocopy.copy(src, dest, patterns)

    local result = 0
    for p = 1, #patterns do

        local copyResult = os.execute("robocopy " .. src .. " " .. dest .. " " .. patterns[p] .. " /e")
        if copyResult ~= 0 then
            result = copyResult
        end

    end

    return result == 0
    
end