# -*- coding: utf-8; mode: tcl; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- vim:fenc=utf-8:ft=tcl:et:sw=4:ts=4:sts=4

PortSystem              1.0
PortGroup               github 1.0
PortGroup               cmake 1.1

name                    gensync
version                 1.0
revision                0
categories              network dev
license                 GPL-3
maintainers             {@trachten bu.edu:trachten} openmaintainer
description             Gensync: a library for network syncing
long_description        Gensync is a library that uses many different syncing
                        algorithms to sync data between two nodes in a network.
                        These algorithms include IBLTs, CPISyncs, HashSyncs, 
                        Cuckoo Syncs, and more.

homepage                https://github.com/nislab/gensync/
platforms               macosx
fetch.type              git
git.url                 https://github.com/nislab/gensync.git
git.branch              master


checksums               

compiler.cxx_standard   2011

depends_lib-append      port:ntl \ port:cppunit \ port:cmake

worksrcdir              ${name}-${version}/src


post-destroot {
    system "cd ${destroot}${prefix}/share/doc && mv NTL tmp && mv tmp ${name}"
    xinstall -m 0644 ${worksrcpath}/../README \
                     ${destroot}${prefix}/share/doc/${name}
    xinstall -m 0644 ${worksrcpath}/../doc/copying.txt \
                     ${destroot}${prefix}/share/doc/${name}/LICENSE
}

variant tuned description {Build with more optimizations} {
    configure.args-delete  NATIVE=off
    configure.args-append  TUNE=auto
}

test.run                yes
test.target             check
