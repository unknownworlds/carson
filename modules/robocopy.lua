robocopy = { }

function robocopy.copy(src, dest, patterns)

    local result = 0
    for p = 1, #patterns do

        local _, _, copyResult = os.execute("robocopy " .. src .. " " .. dest .. " " .. patterns[p] .. " /e")
        if copyResult > result then
            result = copyResult
        end

    end

    return result <= 8
    
end