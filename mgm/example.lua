-- Example to convert latitude/longitude
-- to tile/subtile indicies/coords
-- for MGM map filenames
--
-- Based loosely on this description from JCamp...@gmail.com
-- https://groups.google.com/forum/m/?fromgroups#!topic/google-maps-api/oJkyualxzyY
--
-- For a good description of tiles and zoom levels
-- http://www.mapbox.com/developers/guide/
--
-- Note that math.log is ln

-- home
Lat = 40.061295
Lng = -105.214552

-- compute tile/subtile indices/coords
Zoom       = 14
PI         = 3.141592654
RadLat     = Lat*(PI/180)
RadLng     = Lng*(PI/180)
MercX      = RadLng
MercY      = math.log(math.tan(RadLat) + 1/math.cos(RadLat))
CartX      = MercX + PI
CartY      = PI - MercY
WorldU     = CartX/(2*PI)
WorldV     = CartY/(2*PI)
NumSubTile = 8
TileX      = WorldU*(2^Zoom)/NumSubTile
TileY      = WorldV*(2^Zoom)/NumSubTile
TileIX     = math.floor(TileX)
TileIY     = math.floor(TileY)
SubTileX   = NumSubTile*(TileX - TileIX)
SubTileY   = NumSubTile*(TileY - TileIY)
SubTileIX  = math.floor(SubTileX)
SubTileIY  = math.floor(SubTileY)
SubTileU   = SubTileX - SubTileIX
SubTileV   = SubTileY - SubTileIY

-- sumarize stats
print("WorldU    = " .. WorldU)
print("WorldV    = " .. WorldV)
print("TileIX    = " .. TileIX)
print("TileIY    = " .. TileIY)
print("SubTileIX = " .. SubTileIX)
print("SubTileIY = " .. SubTileIY)
print("SubTileU  = " .. SubTileU)
print("SubTileV  = " .. SubTileV)
