o2 = {}

// Runs func inside a profiler zone named name (seen in Tracy) and returns its result
o2.Profile = function(name, func)
{
    o2.Profiler.Begin(name);
    try
    {
        return func();
    }
    finally
    {
        o2.Profiler.End();
    }
};
