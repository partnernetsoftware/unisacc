GHTAPE1
day=20770
rc=0
ghver=gh version 2.45.0 (2024-03-04)
argc=12
a=run
a=list
a=-R
a=partnernetsoftware/unisacc
a=-w
a=candidate-native.yml
a=--commit
a=fbdc99ca8921514fca927781a85ec93fbb5a82ec
a=--json
a=databaseId,status,conclusion
a=--jq
a=.[]|[.databaseId,.status,.conclusion]|@tsv
len=0
