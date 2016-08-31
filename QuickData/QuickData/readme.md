# QuickData.exe

The data conversion tool used to generate files intended for fast consumption by WiB client.

- Code for app domain logic is in ./
- Data processing code is stored in ./Workflows
- Be sure to use Visual Studio's project view for improved organization
- Currently building against Visual Studio 2013 (Update 5)

Should build into ./bin .


### App Logic

Typically uses procedural to represent complex operations in simple, contained functions. Anything regarding resource tracking is done with RAII.

