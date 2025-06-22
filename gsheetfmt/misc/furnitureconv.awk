# furnitureconv.awk: convert a table that looks like this:
# Room	Desc	Want?	Notes June 2025																						
# Primary bedroom																									
# 	2 end tables	n	JC doesn't like them. Log cabin; not modern		
#
# to:
# FromHouse	FromRoom	Item	Move?	ToHouse	ToRoom	Notes																					
# Applewood	Primary bedroom	Black shelves	n	Applewood	JC doesn't like them. Log cabin; not modern																				
#
# Usage: awk -f furnitureconv.awk existing.tsv > existing-converted.tsv
#
# Mark Riordan  22-JUN-2025
BEGIN {
    FS = "\t"
}
NR > 1 {
    if ($1 != "") {
        roomfrom = $1
    } else {
        housefrom = "Applewood"
        houseto = "Applewood"
        desc = $2
        yesno = $3
        notes = $4
        roomto = ""
        print housefrom "\t" roomfrom "\t" desc "\t" yesno "\t" houseto "\t" roomto "\t" notes
    }
}
