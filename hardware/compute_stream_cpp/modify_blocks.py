import sqlite3

# for row in cur.execute('SELECT * FROM block'):
    # print(row)
con = sqlite3.connect("craft.db")
cur = con.cursor()
to_add = 30
MAGIC = 10000
cur.execute(f'UPDATE block SET y=y+{to_add + MAGIC}')
cur.execute(f'UPDATE block SET y=y-{MAGIC}')
con.commit()
