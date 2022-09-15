var number hidden
vector_get params hidden 0
? hidden 0
je not_hidden
return "******"
label not_hidden
return "SECRET"
