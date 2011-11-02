

def shiftString(string, bits):
    carry = 0
    news = []
    for x in xrange(len(string)-1):
        newc = ((ord(string[x]) << bits) + (ord(string[x+1]) >> (8-bits))) & 0xff
        news.append("%c"%newc)
    newc = (ord(string[-1])<<bits) & 0xff
    news.append("%c"%newc)
    return "".join(news)



def findDword(bytes):
    possDwords = []
    try:
        # find the preamble (if any)
        bitoff = 0
        pidx = bytes.find("\xaa\xaa")
        if pidx == -1:
            pidx = bytes.find("\55\x55")
            bitoff = 1

        # find the definite end of the preamble (ie. it may be sooner, but we know this is the end)
        while (bytes[0] == ('\xaa', '\x55')[bitoff] and len(bytes)>2):
            bytes = bytes[1:]
        # now we look at the next 16 bits to narrow the possibilities to 8
        # at this point we have no hints at bit-alignment
        dwbits, = struct.unpack(">H", bytes[:2])
        if len(bytes)>=6:
            #ddwbits, = struct.unpack(">H", bytes[2:4]
            bitcnt = 0
            #dbt = dwbits
            #ddbt = ddwbits
            #
            #while (dbt != ddbt and bitcnt > 16):
            #    print bin(dbt), bin(ddbt)
            #    bitcnt += 1
            #    ddbt >>= 1
            #    ddbt |= ((dbt&1) << 15)
            #    dbt >>= 1
            #  bits1 =      aaaaaaaaaaaaaaaabbbbbbbbbbbbbbbb
            #  bits2 =                      bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
            bits1, = struct.unpack(">H", bytes[:2])
            bits1 = bits1 | (ord(('\xaa','\x55')[bitoff]) << 16)
            bits1 = bits1 | (ord(('\xaa','\x55')[bitoff]) << 24)
            bits1 <<= 2
            bits1 |= (ord(bytes[2]) >> 6)   # only need two bits please.
            #bits <<= 16
            #bits |= struct.unpack(">H", bytes[4:6])[0]
            bits2, = struct.unpack(">L", bytes[:4])
            bits2 <<= 2
            bits2 |= (ord(bytes[4]) >> 6)   # only need two bits please.

            frontbits = 0
            for frontbits in xrange(18):    #FIXME: if this doesn't work, try 16, then 18+frontbits
                dwb1 = (bits1 >> (16+frontbits)) & 1
                dwb2 = (bits2 >> (16+frontbits)) & 1
                print "\tfrontbits: %d \t\t dwb1: %d dwb2: %d" % (frontbits, dwb1, dwb2)
                if dwb2 != dwb1:
                    break
            # frontbits now represents our unknowns...  let's go from the other side now
            for tailbits in xrange(17, -1, -1):
                dwb1 = (bits1 >> (tailbits)) & 1
                dwb2 = (bits2 >> (tailbits)) & 1
                print "\ttailbits: %d\t\t dwb1: %d dwb2: %d" % (tailbits, dwb1, dwb2)
                if dwb2 != dwb1:
                    break
            # now, if we have a double syncword, iinm, tailbits + frontbits >= 16
            print "frontbits: %d\t\t tailbits: %d, bits: %s %s " % (frontbits, tailbits, bin(bits1), bin(bits2))
            if (frontbits + (18-tailbits) >= 16):
                tbits = bits1 >> tailbits
                tbits &= ((1<<(16+frontbits))-1)
                x = tbits >> 16
                y = tbits & 0xffff
                #x,y = struct.pack(">HH", tbits)
                print "tbits: %x" % tbits
                print "first: %x\t\t second: %x" % (x, y)

    except Exception,e:
        sys.excepthook(*sys.exc_info())

def findDword(byts):
        possDwords = []
        # find the preamble (if any)
        bitoff = 0
        pidx = byts.find("\xaa\xaa")
        if pidx == -1:
            pidx = byts.find("\55\x55")
            bitoff = 1
        if pidx == -1:
            return []

        # chop off the nonsense before the preamble
        byts = byts[pidx:]

        # find the definite end of the preamble (ie. it may be sooner, but we know this is the end)
        while (byts[0] == ('\xaa', '\x55')[bitoff] and len(byts)>2):
            byts = byts[1:]

        # now we look at the next 16 bits to narrow the possibilities to 8
        # at this point we have no hints at bit-alignment
        dwbits, = struct.unpack(">H", byts[:2])
        if len(byts)>=5:
            bitcnt = 0
            #  bits1 =      aaaaaaaaaaaaaaaabbbbbbbbbbbbbbbb
            #  bits2 =                      bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
            bits1, = struct.unpack(">H", byts[:2])
            bits1 = bits1 | (ord(('\xaa','\x55')[bitoff]) << 16)
            bits1 = bits1 | (ord(('\xaa','\x55')[bitoff]) << 24)
            bits1 <<= 8
            bits1 |= (ord(byts[2]) )
            bits2, = struct.unpack(">L", byts[:4])
            bits2 <<= 8
            bits2 |= (ord(byts[4]) )
            frontbits = 0
            for frontbits in xrange(24):    #FIXME: if this doesn't work, try 16, then 18+frontbits
                dwb1 = (bits1 >> (16+frontbits)) & 1
                dwb2 = (bits2 >> (16+frontbits)) & 1
                print "\tfrontbits: %d \t\t dwb1: %d dwb2: %d" % (frontbits, dwb1, dwb2)
                if dwb2 != dwb1:
                    break

            # frontbits now represents our unknowns...  let's go from the other side now
            for tailbits in xrange(23, -1, -1):
                dwb1 = (bits1 >> (tailbits)) & 1
                dwb2 = (bits2 >> (tailbits)) & 1
                print "\ttailbits: %d\t\t dwb1: %d dwb2: %d" % (tailbits, dwb1, dwb2)
                if dwb2 != dwb1:
                    break

            # now, if we have a double syncword, iinm, tailbits + frontbits >= 16
            print "frontbits: %d\t\t tailbits: %d, bits: %s %s " % (frontbits, tailbits, bin(bits1), bin(bits2))
            if (frontbits + (24-tailbits) >= 16):
                tbits = bits1 >> tailbits
                tbits >>= bitoff # yay, we get to use this!
                tbits &= ((1<<(16+frontbits))-1)
                x = tbits >> 16
                y = tbits & 0xffff
                #x,y = struct.pack(">HH", tbits)
                print "tbits: %x" % tbits
                print "first: %x\t\t second: %x" % (x, y)
                while tbits:
                    poss = tbits&0xffff
                    if poss not in possDwords:
                        possDwords.append(tbits&0xffff)
                    tbits>>=2       # because we know the bit offset from the preamble
        return possDwords
