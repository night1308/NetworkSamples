DUMMY:=$(shell rm -rf generated/skippeddependencies)

print=@printf "[32m%40s: %s[0m\n" $@ '$1' >&2
treedep=$(shell find $1 -mindepth 1 --newer $1 | grep -v /ccache- | grep ^ > /dev/null && touch $1) $1

.DELETE_ON_ERROR:

