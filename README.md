# samp-map-collector

This ASI module collects incoming create object and remove building packets from the server, filters out duplicates, and saves it into a PWN (Pawn) file.

#### Supported SA-MP clients
- 0.3.DL-R1

#### Supported functions
- CreateObject
- RemoveBuildingForPlayer

#### Possible upcoming updates
- 0.3.7-R2 support
- 0.3.DL specific custom models
- Object materials
- Attached objects on players and vehicles

#### Installation
1. Download Silent's ASI Loader (https://www.gtagarage.com/mods/show.php?id=21709)
2. Extract it's contents into your GTA: San Andreas installation directory
3. Download samp-map-collector by click on Code dropdown menu and select Download ZIP
4. Extract samp-map-collector.asi module in the ./bin/ directory to your GTA: San Andreas installation directory

#### Usage
It's simple. Join to any SA-MP server and on the connection screen the module already began collecting any created objects and removed buildings. The module will not stop collecting objects until use `/stop_record` command and will not save it into a PWN file until you use `/dump_record` command. See more commands on the Command section.

Be aware that most SA-MP servers uses object streaming plugin to create more than 1000 objects, which means any out of range objects will gets deleted, and only created objects that are near to you. So you'll have to move around the map to collect all of them. 

#### Commands
`/start_record` - Enables incoming data collection, like CreateObject and RemoveBuildingForPlayer, etc...
`/stop_record` - Stops collecting incoming data from the server. This command will not generate any file
`/clear_record` - Clears out all collected data
`/record_stats` - Prints out numbers of collected data
`/dump_record` - Saves all collected data into a PWN file in your GTA:SA directory. This command does not erase collected data, until you use `/clear_record` command
