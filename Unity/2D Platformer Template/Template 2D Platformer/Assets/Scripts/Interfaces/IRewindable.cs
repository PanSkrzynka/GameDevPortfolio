using System.Collections.Generic;
public interface IRewindable
{
    void SaveState(List<object> buffer);
    void LoadState(object state);
}