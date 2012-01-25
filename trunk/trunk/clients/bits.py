import struct

def shiftString(string, bits):
    carry = 0
    news = []
    for x in xrange(len(string)-1):
        newc = ((ord(string[x]) << bits) + (ord(string[x+1]) >> (8-bits))) & 0xff
        news.append("%c"%newc)
    newc = (ord(string[-1])<<bits) & 0xff
    news.append("%c"%newc)
    return "".join(news)

def findDword(byts):
        possDwords = []
        # find the preamble (if any)
        bitoff = 0
        while True:
            sbyts = byts
            pidx = byts.find("\xaa\xaa")
            if pidx == -1:
                pidx = byts.find("\x55\x55")
                bitoff = 1
            if pidx == -1:
                return possDwords
            
            # chop off the nonsense before the preamble
            sbyts = byts[pidx:]
            
            # find the definite end of the preamble (ie. it may be sooner, but we know this is the end)
            while (sbyts[0] == ('\xaa', '\x55')[bitoff] and len(sbyts)>2):
                sbyts = sbyts[1:]
            
            # now we look at the next 16 bits to narrow the possibilities to 8
            # at this point we have no hints at bit-alignment
            dwbits, = struct.unpack(">H", sbyts[:2])
            if len(sbyts)>=3:
                bitcnt = 0
                #  bits1 =      aaaaaaaaaaaaaaaabbbbbbbbbbbbbbbb
                #  bits2 =                      bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
                bits1, = struct.unpack(">H", sbyts[:2])
                bits1 = bits1 | (ord(('\xaa','\x55')[bitoff]) << 16)
                bits1 = bits1 | (ord(('\xaa','\x55')[bitoff]) << 24)
                bits1 <<= 8
                bits1 |= (ord(sbyts[2]) )
                bits1 >>= bitoff            # now we should be aligned correctly

                while (bits1 & 0xf00 != 0xa00): # now we align the end of the 101010 pattern with the beginning of the dword
                    bits1 >>= 2
                
                for frontbits in xrange(0, 16, 2):
                    poss = (bits1 >> frontbits) & 0xffff
                    if not poss in possDwords:
                        possDwords.append(poss)
            byts = byts[pidx+1:]
        
        return possDwords

def findDwordDoubled(byts):
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
            for frontbits in xrange(16, 40):    #FIXME: if this doesn't work, try 16, then 18+frontbits
                dwb1 = (bits1 >> (frontbits)) & 1
                dwb2 = (bits2 >> (frontbits)) & 1
                print "\tfrontbits: %d \t\t dwb1: %s dwb2: %s" % (frontbits, bin(bits1 >> (frontbits)), bin(bits2 >> (frontbits)))
                if dwb2 != dwb1:
                    break

            # frontbits now represents our unknowns...  let's go from the other side now
            for tailbits in xrange(16, -1, -1):
                dwb1 = (bits1 >> (tailbits)) & 1
                dwb2 = (bits2 >> (tailbits)) & 1
                print "\ttailbits: %d\t\t dwb1: %s dwb2: %s" % (tailbits, bin(bits1 >> (tailbits)), bin(bits2 >> (tailbits)))
                if dwb2 != dwb1:
                    tailbits += 1
                    break

            # now, if we have a double syncword, iinm, tailbits + frontbits >= 16
            print "frontbits: %d\t\t tailbits: %d, bits: %s %s " % (frontbits, tailbits, bin(bits1), bin(bits2))
            if (frontbits + tailbits >= 16):
                tbits = bits1 >> (tailbits&0xfffe)
                tbits >>= bitoff # yay, we get to use this!
                tbits &= ((1<<(16+frontbits))-1)
                print "tbits: %x" % tbits

                for bs in xrange(tailbits, frontbits-16, 2):
                    poss = tbits&0xffff
                    if poss not in possDwords:
                        possDwords.append(tbits&0xffff)
                    tbits>>=2       # because we know the bit offset from the preamble
                possDwords.reverse()
            else:
                pass
                # FIXME: what if we *don't* have a double-sync word?  then we stop at AAblah or 55blah and take the next word?
        return possDwords
