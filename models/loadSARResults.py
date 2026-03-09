import sys
from pyravel import DataSpec, RavelDatabase
db=RavelDatabase()
db.connect("sqlite3","db=SARResults.sqlite")

# set up spec for SARResults.csv
spec=DataSpec()
spec.dataRowOffset(1)
spec.dataCols([7,8])
spec.dimensionCols([0,1,2,3,4])
spec.dimensions([
    {"name":"Replicate","dimension":{"type":"value"}},
    {"name":"Area","dimension":{"type":"value"}},
    {"name":"Mutation rate","dimension":{"type":"value"}},
    {"name":"Migration rate","dimension":{"type":"value"}},
    {"name":"Init Num Species","dimension":{"type":"value"}},
    {"name":"Dummy","dimension":{"type":"value"}},
    {"name":"Dummy","dimension":{"type":"value"}},
    {"name":"Nsp","dimension":{"type":"value"}},
    {"name":"s^2","dimension":{"type":"value"}},
])
#spec.dontFail(True)

db.table("SARResults")
db.createTable("SARResults.csv",spec())
db.importFromCSV(["SARResults.csv"],spec())
