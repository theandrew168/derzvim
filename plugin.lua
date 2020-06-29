function f(x, y)
    return (x^2 * math.sin(y)) / (1 - x)
end

function on_line_draw(line)
    return line .. " and some extra stuff from a plugin"
end
