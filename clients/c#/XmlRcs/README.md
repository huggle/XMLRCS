Library for C#

This is a very simple client for XmlRcs service, here is a minimal example console application using this library:

```
    class Program
    {
        static void OnChange(object sender, XmlRcs.EditEventArgs args)
        {
            switch (args.Change.Type)
            {
                case XmlRcs.RecentChange.ChangeType.New:
                    Console.WriteLine("NEW PAGE: " + args.Change.Title + " on wiki " + args.Change.Wiki);
                    break;
                case XmlRcs.RecentChange.ChangeType.Edit:
                    Console.WriteLine("EDIT ON: " + args.Change.Title + " on wiki " + args.Change.Wiki);
                    break;
            }
        }

        static void Main(string[] args)
        {
            Console.WriteLine("Connecting");
            XmlRcs.Provider provider = new XmlRcs.Provider(true);
            provider.Subscribe("en.wikipedia.org");
            
            provider.On_Change += OnChange;
            Console.WriteLine("Connected");
            Console.ReadLine();
        }
    }
```
