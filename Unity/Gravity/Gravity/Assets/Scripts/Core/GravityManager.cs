using System.Collections.Generic;

public static class GravityManager
{
    public static readonly List<GravityObject> Objects = new();

    public static void Register(GravityObject obj)
    {
        if (!Objects.Contains(obj))
            Objects.Add(obj);
    }

    public static void Unregister(GravityObject obj)
    {
        Objects.Remove(obj);
    }
}