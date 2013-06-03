#!/usr/bin/lua

-- Example to convert latitude/longitude
-- to/from tile/subtile indicies/coords
-- for MGM map filenames
--
-- Based loosely on this description from JCamp...@gmail.com
-- https://groups.google.com/forum/m/?fromgroups#!topic/google-maps-api/oJkyualxzyY
--
-- For a good description of tiles and zoom levels
-- http://www.mapbox.com/developers/guide/
--
-- For a description of the Mercator Projection
-- http://en.m.wikipedia.org/wiki/Mercator_projection
--
-- Note that math.log is ln

function coord2tile(Zoom, Lat, Lon)
	local PI         = 3.141592654
	local RadLat     = Lat*(PI/180)
	local RadLon     = Lon*(PI/180)
	local MercX      = RadLon
	local MercY      = math.log(math.tan(RadLat) + 1/math.cos(RadLat))
	local CartX      = MercX + PI
	local CartY      = PI - MercY
	local WorldU     = CartX/(2*PI)
	local WorldV     = CartY/(2*PI)
	local NumSubTile = 8
	local TileX      = WorldU*(2^Zoom)/NumSubTile
	local TileY      = WorldV*(2^Zoom)/NumSubTile
	local TileIX     = math.floor(TileX)
	local TileIY     = math.floor(TileY)
	local SubTileX   = NumSubTile*(TileX - TileIX)
	local SubTileY   = NumSubTile*(TileY - TileIY)
	local SubTileIX  = math.floor(SubTileX)
	local SubTileIY  = math.floor(SubTileY)
	local SubTileU   = SubTileX - SubTileIX
	local SubTileV   = SubTileY - SubTileIY

	-- sumarize stats
	print("-- coord2tile --")
	print("WorldU    = " .. WorldU)
	print("WorldV    = " .. WorldV)
	print("TileIX    = " .. TileIX)
	print("TileIY    = " .. TileIY)
	print("SubTileIX = " .. SubTileIX)
	print("SubTileIY = " .. SubTileIY)
	print("SubTileU  = " .. SubTileU)
	print("SubTileV  = " .. SubTileV)

	return { x=TileX, y=TileY }
end

function tile2coord(Zoom, TileX, TileY)
	local PI         = 3.141592654
	local NumSubTile = 8
	local WorldU     = NumSubTile*TileX/(2^Zoom)
	local WorldV     = NumSubTile*TileY/(2^Zoom)
	local CartX      = 2*PI*WorldU
	local CartY      = 2*PI*WorldV
	local MercX      = CartX - PI
	local MercY      = PI - CartY
	local RadLon     = MercX
	local RadLat     = 2*math.atan(math.exp(MercY)) - PI/2
	local Lat        = RadLat/(PI/180)
	local Lon        = RadLon/(PI/180)

	print("-- tile2coord --")
	print("WorldU = " .. WorldU)
	print("WorldV = " .. WorldV)
	print("Lat    = " .. Lat)
	print("Lon    = " .. Lon)

	return { lat=Lat, lon=Lon }
end

home = { lat=40.061295, lon=-105.214552 }

print("-- home --")
print("Lat = " .. home.lat)
print("Lon = " .. home.lon)

tile  = coord2tile(14, home.lat, home.lon)
coord = tile2coord(14, tile.x, tile.y)
