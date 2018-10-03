#include "GEDCOMutilities.h"
#include "LinkedListAPI.h"
#include <sys/stat.h>
#include <ctype.h>
#include <stdarg.h>

#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_ ## x
#endif

typedef struct {
	Individual individual;
	char id[16];
	List listOfFamiliesIds;
} IndividualWithId;

typedef struct {
	Family family;
	char id[16];
	char* wifeId;
	char* husbandId;
	List childrenIds;
} FamilyWithIds;

typedef struct {
	Header header;
	char submitterId[16];
} HeaderWithSubmitterId;


/////  Dyn Buffer implementation

typedef struct buffer
{
	char* str;
	int allocated;
	int length;
} DynBuffer;

void initDynBuffer(DynBuffer* buffer)
{
	buffer->str = malloc(buffer->allocated = 1024);
	buffer->str[buffer->length = 0] = '\0';
}

void deleteDynBuffer(DynBuffer* buffer)
{
	free(buffer->str);
	buffer->str = 0;
	buffer->length = 0;
}

void appendToDynBuffer(DynBuffer* buffer, const char* data)
{
	int len = strlen(data);

	if (len + buffer->length >= buffer->allocated)
	{
		// reallocate
		int newSize = (len + buffer->length) | 0x400;
		buffer->str = realloc(buffer->str, newSize);
		buffer->allocated = newSize;
	}
	strncat(buffer->str, data, buffer->allocated - len);
}

int growDynBuffer(DynBuffer* buffer)
{
	buffer->allocated += 1024;
	buffer->str = realloc(buffer->str, buffer->allocated);
	return buffer->allocated;
}

int appendIntends(DynBuffer* buffer, int count);

int sprintfDyn(DynBuffer* buffer, int intends, const char* format, ...)
{
	int res;
	if (intends)
	{
		appendIntends(buffer, intends);
	}

	int rest = buffer->allocated - buffer->length;
	va_list args;
	va_start(args, format);

	while ((res = vsnprintf(buffer->str + buffer->length, rest, format, args)) >= rest)
	{
		rest = growDynBuffer(buffer) - buffer->length;
	}
	buffer->length += res;
	return res + intends;
}

int appendIntends(DynBuffer* buffer, int count)
{
	char* buf = malloc(count + 1);
	memset(buf, '\t', count);
	buf[count] = '\0';
	int res = sprintfDyn(buffer, 0, "%s", buf);
	free(buf);
	return res;
}

///////// parsing functions ///////////////

char* readLine(char* content, char** res)
{
	char* lineEnd = content;
	for (; *lineEnd && *lineEnd != '\n' && *lineEnd != '\r' ; lineEnd++) {};

	char* newPos = lineEnd;
	while (*newPos == '\r' || *newPos == '\n')
    {
        newPos++;
    }

	int len = lineEnd - content + 1;
	*res = malloc(len);

	strncpy(*res, content, len);
	(*res)[len - 1] = '\0';

	return newPos;
}

char* skipWord(char* line) {
	for (; *line && !isspace(*line); line++);
	return line;
}

char* skipSpaces(char* line) {
	for (; *line && isspace(*line); line++);
	return line;
}

char* getWord(char* line, int index, char** res) {
	*res = NULL;
	char* pos = line;
	for (int i = 0; i < index && (*pos); i++) {
		pos = skipWord(pos);
		pos = skipSpaces(pos);
	}
	if (!(*pos)) {
		return pos;
	}
	char* end = pos;
	for (; *end && !isspace(*end); end++);
	*res = malloc(end - pos + 1);
	strncpy(*res, pos, end - pos + 1);
	(*res)[end - pos] = 0;
	return end;
}

GEDCOMerror createError(ErrorCode type, int line) {
	GEDCOMerror res;
	res.type = type;
	res.line = line;
	return res;
}

GEDCOMerror getTwoWords(char* line, char** word1, char** word2, ErrorCode errCode) {
	*word1 = NULL;
	*word2 = NULL;
	getWord(line, 1, word1);

	if (!*word1) {
		return createError(errCode, 0);
	}

	getWord(line, 2, word2);

	if (!*word2) {
		free(*word1);
		return createError(errCode, 0);
	}
	return createError(OK, 0);
}

GEDCOMerror readFileToMemory(char* fileName, char** buffer)
{
	*buffer = NULL;
	struct stat st;
	if (stat(fileName, &st))
	{
		return createError(INV_FILE, -1);
	}

	int filesize = st.st_size;
	FILE* fl = fopen(fileName, "rb");

	if (!fl)
	{
		return createError(INV_FILE, -1);
	}

	*buffer = malloc(filesize + 1);

    if (filesize != (int)fread(*buffer, 1, filesize, fl))
	{
		free(*buffer);
		*buffer = NULL;
		return createError(INV_FILE, -1);
	}

	(*buffer)[filesize] = '\0';
	fclose(fl);
	return createError(OK, 0);
}

int comparePointers(const void* data1, const void* data2)
{
	if (data1 == data2)
	{
		return 0;
	}
	return data1 > data2 ? 1 : -1;
}

bool pointersAreEqual(const void* data1, const void* data2) {
    return data1 == data2;
}

void doNotDelete(void* UNUSED(obj)) {
	// if we keep reference without owning
}

char* printIndividual(void* obj) {
	IndividualWithId* indi = (IndividualWithId*)obj;
	DynBuffer buffer;
	initDynBuffer(&buffer);
	sprintfDyn(&buffer, 0, "Given name: %s\n", indi->individual.givenName);
	sprintfDyn(&buffer, 0, "Surname: %s\n", indi->individual.surname);
	// events:
	sprintfDyn(&buffer, 0, "Events: \n");
	ListIterator iter = createIterator(indi->individual.events);
	for (void* data = nextElement(&iter); data; data = nextElement(&iter))
	{
		Event* event = (Event*)data;
		char* buf = printEvent(event);
		sprintfDyn(&buffer, 0, "%s\n", buf);
		free(buf);
	}

	iter = createIterator(indi->individual.otherFields);
	for (void* data = nextElement(&iter); data; data = nextElement(&iter))
	{
		Field* field = (Field*)data;
		sprintfDyn(&buffer, 0, "- %s: %s\n", field->tag, field->value);
	}
	return buffer.str;
}

void deleteIndividual(void* obj) {
	IndividualWithId* indi = (IndividualWithId*)obj;
	clearList(&indi->listOfFamiliesIds);
	clearList(&indi->individual.families);
	clearList(&indi->individual.otherFields);
	clearList(&indi->individual.events);
	free(indi->individual.givenName);
	free(indi->individual.surname);
	free(indi);
}

int compareIndividuals(const void* obj1, const void* obj2) {
	return comparePointers(obj1, obj2);
}

char* printFamily(void* obj) {
	FamilyWithIds* family = (FamilyWithIds*)obj;
	DynBuffer buffer;
	initDynBuffer(&buffer);
	if (family->family.husband) {
		sprintfDyn(&buffer, 0, "husband - %s %s\n", family->family.husband->givenName, family->family.husband->surname);
	}
	if (family->family.wife) {
		sprintfDyn(&buffer, 0, "wife - %s %s\n", family->family.wife->givenName, family->family.wife->surname);
	}
	return buffer.str;
}

void deleteFamily(void* obj) {
	FamilyWithIds* family = (FamilyWithIds*)obj;
	clearList(&family->childrenIds);
	clearList(&family->family.children);
	clearList(&family->family.otherFields);
	clearList(&family->family.events);
	free(family->husbandId);
	free(family->wifeId);
	free(family);
}

int compareFamilies(const void* obj1, const void* obj2) {
	return comparePointers(obj1, obj2);
}

char* printId(void* obj) {
	DynBuffer buffer;
	initDynBuffer(&buffer);
	sprintfDyn(&buffer, 0, "%s", obj);
	return buffer.str;
}

void deleteId(void* obj) {
	free(obj);
}

int compareId(const void* obj1, const void* obj2) {
	return strcmp(obj1, obj2);
}

char* printField(void* obj) {
	Field* field = (Field*)obj;
	DynBuffer buffer;
	initDynBuffer(&buffer);
	sprintfDyn(&buffer, 0, "%s = %s", field->tag, field->value);
	return buffer.str;
}

void deleteField(void* obj) {
	Field* field = (Field*)obj;
	free(field->tag);
	free(field->value);
	free(field);
}

int compareFields(const void* obj1, const void* obj2) {
	return comparePointers(obj1, obj2);
}

char* printEvent(void* obj) {
	Event* event = (Event*)obj;
	DynBuffer buffer;
	initDynBuffer(&buffer);
	sprintfDyn(&buffer, 0, "type = %s\n", event->type);
	if (event->date) {
		sprintfDyn(&buffer, 0, "date = %s\n", event->date);
	}
	if (event->place) {
		sprintfDyn(&buffer, 0, "place = %s\n", event->place);
	}
	ListIterator iter = createIterator(event->otherFields);
	for (void* data = nextElement(&iter); data; data = nextElement(&iter))
	{
		Field* field = (Field*)data;
		sprintfDyn(&buffer, 0, "- %s: %s\n", field->tag, field->value);
	}
	return buffer.str;
}

void deleteEvent(void* obj) {
	Event* event = (Event*)obj;
	if (event->date) {
		free(event->date);
	}
	if (event->place) {
		free(event->place);
	}
	clearList(&event->otherFields);
	free(event);
}

int compareEvents(const void* obj1, const void* obj2) {
	return comparePointers(obj1, obj2);
}

char* printNothig(void* UNUSED(obj)) {
    return NULL;
}

int compareLists(const void* obj1, const void* obj2) {
    return comparePointers(obj1, obj2);
}

void deleteList(void* obj) {
    List* list = (List*)obj;
    clearList(list);
    free(list);
}

void initGEDCOMobject(GEDCOMobject* obj) {
	obj->header = NULL;
	obj->submitter = NULL;
	obj->families = initializeList(&printFamily, &deleteFamily, &compareFamilies);
	obj->individuals = initializeList(&printIndividual, &deleteIndividual, &compareIndividuals);
}

typedef struct {
	void* receiver;
	GEDCOMerror (*enter)(void* receiver, char* line, void* newScope);
} ParserScope;

#define MAX_PARSER_DEEP 8

void initHeader(Header* header) {
	header->source[0] = 0;
	header->gedcVersion = 0.0f;
	header->encoding = -1;
	header->submitter = NULL;
	header->otherFields = initializeList(&printField, &deleteField, &compareFields);
}

GEDCOMerror SkipAllReceiver(void* UNUSED(receiver), char* UNUSED(line), void* newScope) {
	ParserScope* targetScope = (ParserScope*)newScope;
	targetScope->receiver = &SkipAllReceiver;
	return createError(OK, 0);
}

GEDCOMerror HeaderGEDCEnter(void* receiver, char* line, void* newScope) {
	ParserScope* targetScope = (ParserScope*)newScope;
	Header* obj = (Header*)receiver;

	char* word1 = NULL;
	char* word2 = NULL;

	GEDCOMerror res = getTwoWords(line, &word1, &word2, INV_HEADER);
	if (res.type != OK) {
		return res;
	}
	targetScope->enter = &SkipAllReceiver;

	if (!strcmp(word1, "VERS")) {
		// TODO: add validation
		obj->gedcVersion = (float)atof(word2);
	}
	free(word2);
	free(word1);
	return res;
}

GEDCOMerror parseAsField(List* list, char* line, int errCode) {

	char* word1 = NULL;

	char* end = getWord(line, 1, &word1);
	if (!word1) {
		return createError(errCode, 0);
	}

	int valueSize = strlen(end);
	char* value = malloc(valueSize + 1);
	strncpy(value, end, valueSize);
	value[valueSize] = 0;

	Field* field = malloc(sizeof(Field));
	field->tag = word1;
	field->value = value;

	insertBack(list, field);

	return createError(OK, 0);
}

GEDCOMerror parseEncoding(char* text, CharSet* charset) {
	GEDCOMerror res = createError(OK, 0);
	if (!strcmp(text, "UTF-8")) {
		*charset = UTF8;
	} else if (!strcmp(text, "ASCII")) {
		*charset = ASCII;
	} else if (!strcmp(text, "UNICODE")) {
		*charset = UNICODE;
	} else if (!strcmp(text, "ANSEL")) {
		*charset = ANSEL;
	} else {
		res = createError(INV_HEADER, 0);
	}
	return res;
}

GEDCOMerror HeaderEnter(void* receiver, char* line, void* newScope) {
	ParserScope* targetScope = (ParserScope*)newScope;
	HeaderWithSubmitterId* obj = (HeaderWithSubmitterId*)receiver;
	char* word1 = NULL;
	getWord(line, 1, &word1);

	if (!word1) {
		return createError(INV_HEADER, 0);
	}

	GEDCOMerror res = createError(OK, 0);
	targetScope->enter = &SkipAllReceiver;

	if (!strcmp(word1, "GEDC")) {
		targetScope->receiver = obj;
		targetScope->enter = &HeaderGEDCEnter;
	} else if (!strcmp(word1, "SUBM")) {
		char* word2 = NULL;
		getWord(line, 2, &word2);
		if (!word2) {
			res = createError(INV_HEADER, 0);
		} else {
			strncpy(obj->submitterId, word2, sizeof(obj->submitterId));
			free(word2);
		}
	} else if (!strcmp(word1, "SOUR")) {
		char* word2 = NULL;
		getWord(line, 2, &word2);
		if (!word2) {
			res = createError(INV_HEADER, 0);
		} else {
			strncpy(obj->header.source, word2, sizeof(obj->header.source));
			free(word2);
		}
	} else if (!strcmp(word1, "CHAR")) {
		char* word2 = NULL;
		getWord(line, 2, &word2);
		if (!word2) {
			res = createError(INV_HEADER, 0);
		} else {
			res = parseEncoding(word2, &obj->header.encoding);
			free(word2);
		}
	} else {
		parseAsField(&obj->header.otherFields, line, INV_HEADER);
	}

	free(word1);
	return res;
}

void parseNames(char* line, char** givenName, char** surname) {
	getWord(line, 2, givenName);
	if (!*givenName) {
		// empty both
		*givenName = malloc(1);
		(*givenName)[0] = 0;
		*surname = malloc(1);
		(*surname)[0] = 0;
	} else {
		// check what we have got
		if ((*givenName)[0] == '/') {
			// this is surname, givenname is empty
			*surname = *givenName;
			*givenName = malloc(1);
			(*givenName)[0] = 0;
		}
		else {
			// got givenname, check if we have surname
			getWord(line, 3, surname);
			if (!*surname)
			{
				*surname = malloc(1);
				(*surname)[0] = 0;
			}
			else
			{
				int len = strlen(*surname);
				char* newSurname = malloc(len - 1);
				strncpy(newSurname, (*surname) + 1, len - 2);
				newSurname[len - 2] = 0;
				free(*surname);
				*surname = newSurname;
			}
		}
	}
}

const char* EVENT_TAGS[] = {
	"BIRT", "DEAT", "MARR", "CHR", "BURI"
};

bool isStandardEvent(const char* tag) {
    for (int i = 0; i < (int)(sizeof(EVENT_TAGS) / sizeof(char*)); i++) {
		if (!strcmp(tag, EVENT_TAGS[i])) {
			return true;
		}
	}
	return false;
}

GEDCOMerror EnterEvent(void* receiver, char* line, void* newScope) {
	ParserScope* targetScope = (ParserScope*)newScope;
	Event* obj = (Event*)receiver;
	targetScope->enter = &SkipAllReceiver;
	GEDCOMerror res = createError(OK, 0);

	char* word1 = NULL;
	char* end = getWord(line, 1, &word1);
	while (isspace(*end)) end++;
	int len = strlen(end);
	while (end[len - 1] == '\r' || end[len - 1] == '\n') len--;
	char* value = malloc(len + 1);
	strncpy(value, end, len);
	value[len] = 0;

	if (!word1) {
		return createError(INV_RECORD, 0);
	}

	if (!strcmp(word1, "TYPE")) {
		strncpy(obj->type, word1, sizeof(obj->type));
		free(word1);
	} else if (!strcmp(word1, "PLAC")) {
		obj->place = value;
		free(word1);
	} else if (!strcmp(word1, "DATE")) {
		obj->date = value;
		free(word1);
	}
	else {
		Field* field = malloc(sizeof(Field));
		field->tag = word1;
		field->value = value;
		insertBack(&obj->otherFields, field);
	}

	return res;
}

GEDCOMerror IndiEnter(void* receiver, char* line, void* newScope) {
	ParserScope* targetScope = (ParserScope*)newScope;
	IndividualWithId* obj = (IndividualWithId*)receiver;
	targetScope->enter = &SkipAllReceiver;

	char* word1 = NULL;
	getWord(line, 1, &word1);

	if (!word1) {
		return createError(INV_RECORD, 0);
	}

	GEDCOMerror res = createError(OK, 0);

	if (!strcmp(word1, "NAME")) {
		parseNames(line, &obj->individual.givenName, &obj->individual.surname);
	} else if (!strcmp(word1, "FAMS") || !strcmp(word1, "FAMC")) {
		char* familyId = NULL;
		getWord(line, 2, &familyId);
		if (!familyId) {
			res = createError(INV_RECORD, 0);
			goto clearAndReturn;
		}
		insertBack(&obj->listOfFamiliesIds, familyId);
	}
	else if (!strcmp(word1, "EVEN") || isStandardEvent(word1)) {
		Event* event = malloc(sizeof(Event));
		memset(event, 0, sizeof(Event));
		strncpy(event->type, word1, sizeof(event->type));
		insertBack(&obj->individual.events, event);
		targetScope->receiver = event;
		targetScope->enter = &EnterEvent;
	} else {
		parseAsField(&obj->individual.otherFields, line, INV_RECORD);
	}
clearAndReturn:
	free(word1);
	return res;
}

GEDCOMerror FamilyEnter(void* receiver, char* line, void* newScope) {
	ParserScope* targetScope = (ParserScope*)newScope;
	FamilyWithIds* obj = (FamilyWithIds*)receiver;
	targetScope->enter = &SkipAllReceiver;

	char* word1 = NULL;
	getWord(line, 1, &word1);

	if (!word1) {
		return createError(INV_RECORD, 0);
	}
	GEDCOMerror res = createError(OK, 0);
	if (!strcmp(word1, "HUSB")) {
		getWord(line, 2, &obj->husbandId);
		if (!obj->husbandId) {
			res = createError(INV_RECORD, 0);
			goto clearAndReturn;
		}
	} else if (!strcmp(word1, "WIFE")) {
		getWord(line, 2, &obj->wifeId);
		if (!obj->wifeId) {
			res = createError(INV_RECORD, 0);
			goto clearAndReturn;
		}
	} else if (!strcmp(word1, "CHIL")) {
		char* chilId = NULL;
		getWord(line, 2, &chilId);
		if (!chilId) {
			res = createError(INV_RECORD, 0);
			goto clearAndReturn;
		}
		insertBack(&obj->childrenIds, chilId);
	} else if (!strcmp(word1, "EVEN") || isStandardEvent(word1)) {
		Event* event = malloc(sizeof(Event));
		memset(event, 0, sizeof(Event));
		strncpy(event->type, word1, sizeof(event->type));
		insertBack(&obj->family.events, event);
		targetScope->receiver = event;
		targetScope->enter = &EnterEvent;
	} else {
		parseAsField(&obj->family.otherFields, line, INV_RECORD);
	}
clearAndReturn:
	free(word1);
	return res;
}

GEDCOMerror SubmitterAddressReceiver(void* receiver, char* line, void* newScope) {
	ParserScope* targetScope = (ParserScope*)newScope;
	GEDCOMobject* obj = (GEDCOMobject*)receiver;
	targetScope->enter = &SkipAllReceiver;

	char* word1 = NULL;

	char* end = getWord(line, 1, &word1);
	if (!word1) {
		return createError(INV_RECORD, 0);
	}
	int valueSize = strlen(end);
	char* value = malloc(valueSize + 1);
	strncpy(value, end, valueSize);
	value[valueSize] = 0;

	int currentSize = strlen(obj->submitter->address);
	bool replaceHeaderSubmitter = obj->submitter == obj->header->submitter;
	obj->submitter = realloc(obj->submitter, sizeof(Submitter) + currentSize + strlen(word1) + strlen(value) + 5);
	if (replaceHeaderSubmitter) {
		obj->header->submitter = obj->submitter;
	}
	sprintf(obj->submitter->address + currentSize, "\n%s = %s",word1, value);
	free(word1);
	free(value);
	return createError(OK, 0);
}

GEDCOMerror EnterSubmitter(void* receiver, char* line, void* newScope) {
	ParserScope* targetScope = (ParserScope*)newScope;
	GEDCOMobject* obj = (GEDCOMobject*)receiver;
	targetScope->enter = &SkipAllReceiver;

	char* word1 = NULL;
	char* end = getWord(line, 1, &word1);

	if (!word1) {
		return createError(INV_RECORD, 0);
	}

	GEDCOMerror res = createError(OK, 0);

	if (!strcmp(word1, "NAME")) {
		char* name = NULL;
		getWord(line, 2, &name);
		if (!name) {
			res = createError(INV_RECORD, 0);
			goto clearAndReturn;
		}
		strncpy(obj->submitter->submitterName, name, sizeof(obj->submitter->submitterName));
		free(name);
	}
	else if (!strcmp(word1, "ADDR")) {
		/*
		1 ADDR Address Line 1
		2 CONT Address Line 2
		2 CONT Address Line 3
		2 CONT Address Line 4
		2 CTRY Country
		*/
		int valueSize = strlen(end);
		char* value = malloc(valueSize + 1);
		strncpy(value, end, valueSize);
		value[valueSize] = 0;

		int currentSize = strlen(obj->submitter->address);
		bool replaceHeaderSubmitter = obj->submitter == obj->header->submitter;
		obj->submitter = realloc(obj->submitter, sizeof(Submitter) + currentSize + strlen(word1) + strlen(value) + 5);
		if (replaceHeaderSubmitter) {
			obj->header->submitter = obj->submitter;
		}
		sprintf(obj->submitter->address + currentSize, "%s = %s", word1, value);
		free(value);

		targetScope->enter = &SubmitterAddressReceiver;
		targetScope->receiver = obj;

	} else {
		parseAsField(&obj->submitter->otherFields, line, INV_RECORD);
	}

clearAndReturn:
	free(word1);
	return res;
}

GEDCOMerror GEDCOMobjectEnter(void* receiver, char* line, void* newScope) {
	ParserScope* targetScope = (ParserScope*)newScope;
	GEDCOMobject* obj = (GEDCOMobject*)receiver;
	char* word = NULL;
	char* word2 = NULL;
	getWord(line, 1, &word);

	if (!word) {
		return createError(INV_GEDCOM, 0);
	}

	GEDCOMerror res = createError(OK, 0);
	if (!strcmp(word, "HEAD")) {
		// initialize header
		obj->header = malloc(sizeof(HeaderWithSubmitterId));
		((HeaderWithSubmitterId*)obj->header)->submitterId[0] = '\0';
		initHeader(obj->header);
		targetScope->receiver = obj->header;
		targetScope->enter = &HeaderEnter;
		goto clearAndReturn;
	}

	if (!obj->header) {
		res = createError(INV_GEDCOM, -1);
		goto clearAndReturn;
	}

	if (!obj->header->source[0] || obj->header->gedcVersion == 0.0f || ((int)obj->header->encoding < 0)) { 
		res = createError(INV_HEADER, -1);
		goto clearAndReturn;
	} 
	// there must be second word
	getWord(line, 2, &word2);

	if (!strcmp(word2, "INDI")) {
		// process individual
		IndividualWithId* indi = malloc(sizeof(IndividualWithId));
		indi->listOfFamiliesIds = initializeList(&printId, &deleteId, &compareId);
		indi->individual.families = initializeList(&printFamily, &doNotDelete, &compareFamilies);
		indi->individual.otherFields = initializeList(&printField, &deleteField, &compareFields);
		indi->individual.events = initializeList(&printEvent, &deleteEvent, &compareEvents);
		strncpy(indi->id, word, sizeof(indi->id));
		insertBack(&obj->individuals, indi);
		targetScope->receiver = indi;
		targetScope->enter = &IndiEnter;
	}
	if (!strcmp(word2, "FAM")) {
		FamilyWithIds* family = malloc(sizeof(FamilyWithIds));
		family->husbandId = NULL;
		family->wifeId = NULL;
		family->family.husband = NULL;
		family->family.wife = NULL;
		family->family.children = initializeList(&printIndividual, &doNotDelete, &compareIndividuals);
		family->family.otherFields = initializeList(&printField, &deleteField, &compareFields);
		family->childrenIds = initializeList(&printId, &deleteId, &compareId);
		family->family.events = initializeList(&printEvent, &deleteEvent, &compareEvents);
		strncpy(family->id, word, sizeof(family->id));
		insertBack(&obj->families, family);
		targetScope->receiver = family;
		targetScope->enter = &FamilyEnter;
	}
	if (!strcmp(word2, "SUBM")) {
		obj->submitter = malloc(sizeof(Submitter) + 1);
		obj->submitter->otherFields = initializeList(&printField, &deleteField, &compareFields);
		obj->submitter->address[0] = 0;
		targetScope->receiver = obj;
		targetScope->enter = &EnterSubmitter;
		if (!strcmp(word, ((HeaderWithSubmitterId*)obj->header)->submitterId)) {
			obj->header->submitter = obj->submitter;
		}
	}

	if (!word2) {
		res = createError(INV_GEDCOM, 0);
		goto clearAndReturn;
	}

clearAndReturn:
	free(word);
	if (word2) {
		free(word2);
	}
	return res;
}

bool IndiHasId(const void* p1, const void* p2) {
	IndividualWithId* indi = (IndividualWithId*)p1;
	const char* id = (const char*)p2;
	return !strcmp(indi->id, id);
}

IndividualWithId* findIndiById(List list, const char* id) {
	return findElement(list, &IndiHasId, id);
}

bool FamilyHasId(const void* p1, const void* p2) {
	FamilyWithIds* family = (FamilyWithIds*)p1;
	const char* id = (const char*)p2;
	return !strcmp(family->id, id);
}

FamilyWithIds* findFamilyById(List list, const char* id) {
	return findElement(list, &FamilyHasId, id);
}

GEDCOMerror createGEDCOM(char* fileName, GEDCOMobject** obj) {
	if (!fileName) {
		 return createError(INV_FILE, -1);
	}
	int len = strlen(fileName);
	if (len < 4) {
		return createError(INV_FILE, -1);
	}
	char *ext = fileName + len - 4;
	if (strcmp(ext, ".ged")) {
		return createError(INV_FILE, -1);
	}

	*obj = NULL;
	char* content = NULL;

	GEDCOMerror res;
	if ((res = readFileToMemory(fileName, &content)).type != OK) {
		return res;
	}

	char* position = content;
	ParserScope* scopeStack;
	ParserScope* currentScope;

	*obj = malloc(sizeof(GEDCOMobject));
	initGEDCOMobject(*obj);

	scopeStack = malloc(MAX_PARSER_DEEP * sizeof(ParserScope));
	currentScope = scopeStack;
	currentScope->receiver = *obj;
	currentScope->enter = &GEDCOMobjectEnter;

	char* line = NULL;

	res = createError(INV_GEDCOM, -1);
	bool init = false;
	int lineNum = 1;
	int prevLevel = -1;
	while (*position) {
		position = readLine(position, &line);
		if (!strncmp(line, "0 TRLR", 6)) {
			res = createError(OK, 0);
			break;
		}
		if (strlen(line) > 255) {
			res = createError(INV_RECORD, lineNum);
			goto doExit;			
		}
		int level = atoi(line);
		if (level && !init)
		{
			res = createError(INV_HEADER, -1);
			goto doExit;
		}
		if (level < 0 || level > prevLevel + 1) {
			res = createError(INV_RECORD, lineNum);
			goto doExit;			
		}
		currentScope = scopeStack + level;
		res = currentScope->enter(currentScope->receiver, line, currentScope + 1);
		if (res.type != OK) {
			goto doExit;
		}
		free(line);
		line = NULL;
		init = true;
		prevLevel = level;
		res = createError(INV_GEDCOM, -1);
		lineNum++;
	}

	if (!(*obj)->submitter) {
		res = createError(INV_GEDCOM, -1);
		goto doExit;
	}

	if (!(*obj)->header->submitter) {
		res = createError(INV_HEADER, -1);
		goto doExit;
	}

	// resolve links
	// iterate indies
	ListIterator iter = createIterator((*obj)->individuals);
	for (void* data = nextElement(&iter); data; data = nextElement(&iter))
	{
		IndividualWithId* indi = (IndividualWithId*)data;
		ListIterator familyIdIter = createIterator(indi->listOfFamiliesIds);
		for (void* fdata = nextElement(&familyIdIter); fdata; fdata = nextElement(&familyIdIter))
		{
			const char* familyId = (const char*)fdata;
			FamilyWithIds* family = findFamilyById((*obj)->families, familyId);
			if (!family) {
				deleteGEDCOM(*obj);
				res = createError(INV_GEDCOM, 0);
				goto doExit;
			}
			insertBack(&indi->individual.families, family);
		}
	}

	// iterate families
	iter = createIterator((*obj)->families);
	for (void* data = nextElement(&iter); data; data = nextElement(&iter))
	{
		FamilyWithIds* family = (FamilyWithIds*)data;
		if (family->husbandId) {
			family->family.husband = (Individual*)findIndiById((*obj)->individuals, family->husbandId);
			if (!family->family.husband) {
				deleteGEDCOM(*obj);
				res = createError(INV_GEDCOM, 0);
				goto doExit;
			}
		}
		if (family->wifeId) {
			family->family.wife = (Individual*)findIndiById((*obj)->individuals, family->wifeId);
			if (!family->family.wife) {
				deleteGEDCOM(*obj);
				res = createError(INV_GEDCOM, 0);
				goto doExit;
			}
		}
		ListIterator childIdIter = createIterator(family->childrenIds);
		for (void* cdata = nextElement(&childIdIter); cdata; cdata = nextElement(&childIdIter))	{
			const char* childId = (const char*)cdata;
			IndividualWithId* child = findIndiById((*obj)->individuals, childId);
			if (!child) {
				deleteGEDCOM(*obj);
				res = createError(INV_GEDCOM, 0);
				goto doExit;
			}
			insertBack(&family->family.children, child);
		}
	}
doExit:
	free(content);
	free(scopeStack);
	if (line) {
		free(line);
	}
	if (res.type != OK ) {
		deleteGEDCOM(*obj);
		*obj = NULL;
	}
	return res;
}

void deleteGEDCOM(GEDCOMobject* obj) {
	// delete header
	if (obj->header) {
		clearList(&obj->header->otherFields);
		free(obj->header);
	}
	clearList(&obj->families);
	clearList(&obj->individuals);
	if (obj->submitter) {
		clearList(&obj->submitter->otherFields);
		free(obj->submitter);
	}
	free(obj);
}

char* printSubmitter(Submitter* submitter) {
	DynBuffer buffer;
	initDynBuffer(&buffer);
	sprintfDyn(&buffer, 0, "Name: %s\n", submitter->submitterName);
	sprintfDyn(&buffer, 0, "Address: %s\n", submitter->address);
	if (getLength(submitter->otherFields)) {
		char* buf = toString(submitter->otherFields);
		sprintfDyn(&buffer, 0, "\tFields: %s\n", buf);
		free(buf);
	}
	return buffer.str;
}

char* printGEDCOM(const GEDCOMobject* obj) {
	if (!obj) {
		return NULL;
	}
	DynBuffer buffer;
	initDynBuffer(&buffer);
	sprintfDyn(&buffer, 0, "Header:\n");
	sprintfDyn(&buffer, 0, " - source: %s\n", obj->header->source);
	sprintfDyn(&buffer, 0, " - version: %f\n", obj->header->gedcVersion);
	sprintfDyn(&buffer, 0, " - encoding: %d\n", obj->header->encoding);
	if (obj->header->submitter) {
		char* buf = printSubmitter(obj->header->submitter);
		sprintfDyn(&buffer, 0, "\tSubmitter: %s\n", buf);
		free(buf);
	}

	if (getLength(obj->header->otherFields)) {
		char* buf = toString(obj->header->otherFields);
		sprintfDyn(&buffer, 0, "\tFields: %s\n", buf);
		free(buf);
	}

	char* buf = toString(obj->individuals);
	sprintfDyn(&buffer, 0, "Individuals: %s\n", buf);
	free(buf);

	buf = toString(obj->families);
	sprintfDyn(&buffer, 0, "Families: %s\n", buf);
	free(buf);

	return buffer.str;
}

char* printError(GEDCOMerror err) {
	// OK, INV_FILE, INV_GEDCOM, INV_HEADER, INV_RECORD, OTHER
	char* constands[] = {
		"OK", "Invalid file", "Invalid Gedcom", "Invalid header", "Invalid Record", "Other"
	};
	if (err.type == OK) {
		char* res = malloc(3);
		strncpy(res, "OK", 3);
		return res;
	}
	char* resContant = constands[err.type - OK];
	char* res = malloc(strlen(resContant) + 32);
	sprintf(res, "%s at line %d", resContant, err.line);
	return res;
}

Individual* findPerson(const GEDCOMobject* familyRecord, bool(*compare)(const void* first, const void* second), const void* person) {
	if (!person) {
		return NULL;
	}
	return findElement(familyRecord->individuals, compare, person);
}

void addAll(List* target, List source) {
	ListIterator iter = createIterator(source);
	for (void* data = nextElement(&iter); data; data = nextElement(&iter)) {
		insertBack(target, data);
	}
}

#define LEVELS_LIMIT 100

List descendantsList(const GEDCOMobject* familyRecord, List generation, int level) {
	List res = initializeList(&printIndividual, &deleteIndividual, &compareIndividuals);
	if (level >= LEVELS_LIMIT) {
		return res;
	}
	ListIterator personIterator = createIterator(generation);
	for (void* data = nextElement(&personIterator); data; data = nextElement(&personIterator)) {
		Individual* indi = (Individual*)data;
		// find families where indi wife or husband
		// we cannot use fildElement because there could be several families
		ListIterator familyIter = createIterator(familyRecord->families);
		for (void* data2 = nextElement(&familyIter); data2; data2 = nextElement(&familyIter)) {
			Family* family = (Family*)data2;
			if ((family->wife == indi || family->husband == indi) && getLength(family->children)) {
				// add all
				addAll(&res, family->children);
				List deeply = descendantsList(familyRecord, family->children, level + 1);
				addAll(&res, deeply);
			}
		}
    }
	return res;
}

List getDescendants(const GEDCOMobject* familyRecord, const Individual* person) {
	List res = initializeList(&printIndividual, &deleteIndividual, &compareIndividuals);

	// prepare list for recursion
	insertBack(&res, (void*)person);

	res = descendantsList(familyRecord, res, 0);

	return res;
}

const char* endodingToStr(CharSet charset) {
    const char* names[] = {
      "UTF8", "UNICODE", "ASCII"
    };
    return names[charset - UTF8];
}

GEDCOMerror writeFields(FILE* file, List list, const char* level) {
    ListIterator it = createIterator(list);
    for (void* data = nextElement(&it); data; data = nextElement(&it)) {
        Field* field = (Field*)data;
        if (0 > fprintf(file, "%s %s %s\n", level, field->tag, field->value)) {
            return createError(INV_FILE, 0);
        }
    }
    return createError(OK, 0);
}

GEDCOMerror writeHeader(FILE* file, const Header* obj) {
    if (0 > fprintf(file, "0 HEAD\n")) {
        return createError(INV_FILE, 0);
    }
    if (0 > fprintf(file, "1 SOUR %s\n", obj->source)) {
        return createError(INV_FILE, 0);
    }
    if (0 > fprintf(file, "1 GEDC\n")) {
        return createError(INV_FILE, 0);
    }
    if (0 > fprintf(file, "2 VERS %f\n", obj->gedcVersion)) {
        return createError(INV_FILE, 0);
    }
    if (obj->encoding) {
        if (0 > fprintf(file, "1 CHAR %s\n", endodingToStr(obj->encoding))) {
            return createError(INV_FILE, 0);
        }
    }
    if (obj->submitter) {
        if (0 > fprintf(file, "1 SUBM @U1@\n")) {
            return createError(INV_FILE, 0);
        }
    }
    return writeFields(file, obj->otherFields, "1");
}

int getFamilyIndex(Family* family, Family** families, int familiesCount) {
    for (int i = 0; i < familiesCount; i++) {
        if (family == families[i]) {
            return i;
        }
    }
    return -1;
}

int getIndividualIndex(Individual* indi, Individual** indies, int indisCount) {
    for (int i = 0; i < indisCount; i++) {
        if (indi == indies[i]) {
            return i;
        }
    }
    return -1;
}

GEDCOMerror writeEvents(FILE* file, List events) {
    ListIterator it = createIterator(events);
    for (void* data = nextElement(&it); data; data = nextElement(&it)) {
        Event* event = (Event*)data;
        if (0 > fprintf(file, "1 %s\n", event->type)) {
            return createError(INV_FILE, 0);
        }
        if (event->date) {
            if (0 > fprintf(file, "2 DATE %s\n", event->date)) {
                return createError(INV_FILE, 0);
            }
        }
        if (event->place) {
            if (0 > fprintf(file, "2 PLAC %s\n", event->place)) {
                return createError(INV_FILE, 0);
            }
        }
        GEDCOMerror res = writeFields(file, event->otherFields, "2");
        if (res.type != OK) {
            return res;
        }
    }
    return createError(OK, 0);
}

GEDCOMerror writeIndi(FILE* file, Individual* indi, const char* id, Family** families, int familiesCount) {
    if (0 > fprintf(file, "0 %s INDI\n", id)) {
        return createError(INV_FILE, 0);
    }
    if (0 > fprintf(file, "1 NAME %s /%s/\n", indi->givenName, indi->surname)) {
        return createError(INV_FILE, 0);
    }
    // write families
    ListIterator it = createIterator(indi->families);
    for (void* data = nextElement(&it); data; data = nextElement(&it)) {
        Family* family = (Family*)data;
        int index = getFamilyIndex(family, families, familiesCount);
        char id[16];
        sprintf(id, "@F%d@", index);
        if (0 > fprintf(file, "1 FAMS %s\n", id)) {
            return createError(INV_FILE, 0);
        }
    }
    GEDCOMerror res = writeEvents(file, indi->events);
    if (res.type != OK) {
        return res;
    }
    res = writeFields(file, indi->otherFields, "1");
    return res;
}

GEDCOMerror writeFamily(FILE* file, Family* family, const char* id, Individual** individuals, int individualsCount) {
    if (0 > fprintf(file, "0 %s FAM\n", id)) {
        return createError(INV_FILE, 0);
    }
    if (family->husband) {
        int index = getIndividualIndex(family->husband, individuals, individualsCount);
        char id[16];
        sprintf(id, "@I%d@", index);
        if (0 > fprintf(file, "1 HUSB %s\n", id)) {
            return createError(INV_FILE, 0);
        }
    }
    if (family->wife) {
        int index = getIndividualIndex(family->wife, individuals, individualsCount);
        char id[16];
        sprintf(id, "@I%d@", index);
        if (0 > fprintf(file, "1 WIFE %s\n", id)) {
            return createError(INV_FILE, 0);
        }
    }
    ListIterator it = createIterator(family->children);
    for (void* data = nextElement(&it); data; data = nextElement(&it)) {
        Individual* chil = (Individual*)data;
        int index = getIndividualIndex(chil, individuals, individualsCount);
        char id[16];
        sprintf(id, "@I%d@", index);
        if (0 > fprintf(file, "1 CHIL %s\n", id)) {
            return createError(INV_FILE, 0);
        }
    }
    GEDCOMerror res = writeEvents(file, family->events);
    if (res.type != OK) {
        return res;
    }
    res = writeFields(file, family->otherFields, "1");
    return res;
}

GEDCOMerror writeSubmitter(FILE* file, Submitter* submitter) {
    if (0 > fprintf(file, "0 @U1@ SUBM\n")) {
        return createError(INV_FILE, 0);
    }
    if (0 > fprintf(file, "1 NAME %s\n", submitter->submitterName)) {
        return createError(INV_FILE, 0);
    }
    // write address
    char* line = submitter->address;
    while (*line) {
        char* lineEnd = line;
        for (;*lineEnd && *lineEnd != '\n'; lineEnd++);
        int len = lineEnd - line;
        char* buf = alloca(len + 1);
        strncpy(buf, line, len);
        buf[len] = 0;
        if (line == submitter->address)
        {
            if (0 > fprintf(file, "1 ADDR %s\n", buf)) {
                return createError(INV_FILE, 0);
            }
        }
        else
        {
            if (0 > fprintf(file, "2 CONT %s\n", buf)) {
                return createError(INV_FILE, 0);
            }
        }
        if (!*lineEnd) {
            break;
        }
        line = lineEnd + 1;
    }
    return writeFields(file, submitter->otherFields, "1");
}


GEDCOMerror writeGEDCOM(char* fileName, const GEDCOMobject* obj) {
    FILE* file = fopen(fileName, "w");
    if (!file) {
        return createError(INV_FILE, 0);
    }
    GEDCOMerror res = writeHeader(file, obj->header);
    if (res.type != OK) {
        goto clearAndExit;
    }
    // copy list to array
    Individual** indies = malloc(sizeof(Individual*) * getLength(obj->individuals));
    ListIterator it = createIterator(obj->individuals);
    int counter = 0;
    for (void* data = nextElement(&it); data; data = nextElement(&it)) {
        Individual* indi = (Individual*)data;
        indies[counter++] = indi;
    }
    int indiesCount = counter;
    // and families
    Family** families = malloc(sizeof(Family*) * getLength(obj->families));
    it = createIterator(obj->families);
    for (void* data = nextElement(&it); data; data = nextElement(&it)) {
        Family* family = (Family*)data;
        families[counter++] = family;
    }
    int familiesCount = counter;

    for (int i = 0; i < indiesCount; i++) {
        char id[32];
        sprintf(id, "@I%d@", i + 1);
        writeIndi(file, indies[i], id, families, familiesCount);
    }
clearAndExit:
    fclose(file);
    return res;
}

ErrorCode validateGEDCOM(const GEDCOMobject* obj) {
    if (!obj) {
        return INV_GEDCOM;
    }
    if (!obj->header) {
        return INV_HEADER;
    }
    if (!obj->header->submitter) {
        return INV_HEADER;
    }
    if (!obj->header->submitter->submitterName[0]) {
        return INV_RECORD;
    }
    if (obj->header->gedcVersion == 0.0f) {
        return INV_HEADER;
    }
    if (!obj->submitter) {
        return INV_GEDCOM;
    }
    if (!obj->submitter->submitterName[0]) {
        return INV_RECORD;
    }
    ListIterator it = createIterator(obj->families);
    it = createIterator(obj->families);
    for (void* data = nextElement(&it); data; data = nextElement(&it)) {
        if (!data) {
            return INV_RECORD;
        }
    }
    it = createIterator(obj->individuals);
    for (void* data = nextElement(&it); data; data = nextElement(&it)) {
        if (!data) {
            return INV_RECORD;
        }
    }

    return OK;
}

void addDescendant(const GEDCOMobject* familyRecord, const Individual* person, List** resultAsArray, int currentLevel, int maxGen) {
    // enum families
    ListIterator it = createIterator(familyRecord->families);
    for (void* data = nextElement(&it); data; data = nextElement(&it)) {
        Family* family = (Family*)data;
        if (family->husband == person || family->wife == person) {
            // initialize list if required
            if (!resultAsArray[currentLevel]) {
                resultAsArray[currentLevel] = malloc(sizeof(List));
                *resultAsArray[currentLevel] = initializeList(&printIndividual, &deleteIndividual, &compareIndividuals);
            }
            List* list = resultAsArray[currentLevel];
            ListIterator jt = createIterator(family->children);
            for (void* ch = nextElement(&jt); ch; ch = nextElement(&jt)) {
                Individual* child = (Individual*)ch;
                if (!findElement(*list, &pointersAreEqual, child)) {
                    insertBack(list, ch);
                    // recursion
                    if (currentLevel < maxGen - 1) {
                        addDescendant(familyRecord, child, resultAsArray, currentLevel + 1, maxGen);
                    }
                }
            }
            if (!getLength(*list)) {
                clearList(list);
                resultAsArray[currentLevel] = NULL;
            }
        }
    }
}

List getDescendantListN(const GEDCOMobject* familyRecord, const Individual* person, unsigned int maxGen) {
    List** resultAsArray = malloc(sizeof(List*) * maxGen);
    memset(resultAsArray, 0, sizeof(List*) * maxGen);
    addDescendant(familyRecord, person, resultAsArray, 0, maxGen);
    List res = initializeList(&printGeneration, &deleteGeneration, &compareGenerations);
    for (int i = 0; i < (int)maxGen; i++) {
        if (!resultAsArray[i]) {
            break;
        }
        insertBack(&res, resultAsArray[i]);
    }
    return res;
}

void addAncestor(const GEDCOMobject* familyRecord, const Individual* person, List** resultAsArray, int currentLevel, int maxGen) {
    ListIterator it = createIterator(familyRecord->families);
    for (void* data = nextElement(&it); data; data = nextElement(&it)) {
        Family* family = (Family*)data;
        if (findElement(family->children, &pointersAreEqual, person)) {
            // initialize list if required
            if (!resultAsArray[currentLevel]) {
                resultAsArray[currentLevel] = malloc(sizeof(List));
                *resultAsArray[currentLevel] = initializeList(&printIndividual, &deleteIndividual, &compareIndividuals);
            }
            List* list = resultAsArray[currentLevel];
            if (family->husband) {
                if (!findElement(*list, &pointersAreEqual, family->husband)) {
                    insertBack(list, family->husband);
                    if (currentLevel < maxGen - 1) {
                        addAncestor(familyRecord, family->husband, resultAsArray, currentLevel + 1, maxGen);
                    }
                }
            }
            if (family->wife) {
                if (!findElement(*list, &pointersAreEqual, family->wife)) {
                    insertBack(list, family->wife);
                    if (currentLevel < maxGen - 1) {
                        addAncestor(familyRecord, family->wife, resultAsArray, currentLevel + 1, maxGen);
                    }
                }
            }
            if (!getLength(*list)) {
                clearList(list);
                resultAsArray[currentLevel] = NULL;
            }
        }
    }
}

List getAncestorListN(const GEDCOMobject* familyRecord, const Individual* person, int maxGen) {
    List** resultAsArray = malloc(sizeof(List*) * maxGen);
    memset(resultAsArray, 0, sizeof(List*) * maxGen);
    addAncestor(familyRecord, person, resultAsArray, 0, maxGen);
    List res = initializeList(&printGeneration, &deleteGeneration, &compareGenerations);
    for (int i = 0; i < maxGen; i++) {
        if (!resultAsArray[i]) {
            break;
        }
        insertBack(&res, resultAsArray[i]);
    }
    return res;
}

char* escapeString(const char* s) {
    int len = strlen(s);
    char* res = malloc(len * 2 + 1);
    char* target = res;
    for (const char* src = s; *src; src++) {
        if (*src == '\"') {
            *target++ = '\\';
            *target++ = '\"';
        } else {
            *target++ = *src;
        }
    }
    *target = 0;
    return res;
}

char* indToJSON(const Individual* ind) {
    if (!ind) {
        char* res = malloc(1);
        res[0] = 0;
        return res;
    }
    int len = 0;
    char* escapedGivenName = NULL;
    if (ind->givenName) {
        escapedGivenName = escapeString(ind->givenName);
        len += strlen(escapedGivenName);
    }
    char* escapedSurname = NULL;
    if (ind->surname) {
        escapedSurname = escapeString(ind->surname);
        len += strlen(escapedSurname);
    }
    len += 64;
    char* res = malloc(len);
    sprintf(res, "{\"givenName\":\"%s\",\"surname\":\"%s\"}",
            escapedGivenName ? escapedGivenName : "\"\"",
            escapedSurname ? escapedSurname : "\"\"");
    if (escapedGivenName) {
        free(escapedGivenName);
    }
    if (escapedSurname) {
        free(escapedSurname);
    }
    return res;
}

char* readJsonString(const char** cursor) {
    // *cursor[0] must be '"'
    const char* pos = *cursor;
    pos++;
    int actualLen = 0;
    for (; *pos != '\"'; actualLen++) {
        if (*pos == '\\') {
            pos += 2;
        } else {
            pos++;
        }
    }
    char* res = malloc(actualLen + 1);
    char* target = res;
    for (; *pos != '\"'; actualLen++) {
        if (*pos == '\\') {
            *target++ = *(pos + 1);
            pos += 2;
        } else {
            *target++ = *pos++;
        }
    }
    *target = '\0';
    *cursor = pos;
    return res;
}

Individual* JSONtoInd(const char* str) {
    Individual* res = malloc(sizeof(Individual));
    for (const char* current = str + 1; *current != '}';) {
        char* fieldName = readJsonString(&current);
        // current must refer to ":"
        current++;
        char* value = readJsonString(&current);
        // current is ',' or '}'
        if (!strcmp("givenName", fieldName)) {
            res->givenName = value;
        } else if (!strcmp("surname", fieldName)) {
            res->surname = value;
        } else {
            free(value);
        }
        free(fieldName);
    }
    return res;
}

GEDCOMobject* JSONtoGEDCOM(const char* str) {
    GEDCOMobject* res = malloc(sizeof(GEDCOMobject));
    initGEDCOMobject(res);
    res->submitter = malloc(sizeof(Submitter));
    memset(res->submitter, 0, sizeof(Submitter));
    res->submitter->otherFields = initializeList(&printField, &deleteField, &compareFields);
    for (const char* current = str + 1; *current != '}';) {
        char* fieldName = readJsonString(&current);
        // current must refer to ":"
        current++;
        char* value = readJsonString(&current);
        // current is ',' or '}'
        if (!strcmp("source", fieldName)) {
            strncpy(res->header->source, value, sizeof(res->header->source));
            free(value);
        } else if (!strcmp("gedcVersion", fieldName)) {
            res->header->gedcVersion = atof(value);
            free(value);
        } else if (!strcmp("encoding", fieldName)) {
            parseEncoding(value, &res->header->encoding);
            free(value);
        } else if (!strcmp("subName", fieldName)) {
            strncpy(res->submitter->submitterName, value, sizeof(res->submitter->submitterName));
            free(value);
        } else if (!strcmp("subAddress", fieldName)) {
            int len = strlen(value);
            res->submitter = realloc(res->submitter, sizeof(Submitter) + len + 1);
            strncpy(res->submitter->address, value, len);
            free(value);
        } else {
            free(value);
        }
        free(fieldName);
    }
    res->header->submitter = res->submitter;
    return res;
}

void addIndividual(GEDCOMobject* obj, const Individual* toBeAdded) {
    if (!obj || !toBeAdded) {
        return;
    }
    insertBack(&obj->individuals, (void*)toBeAdded);
}

char* iListToJSON(List iList) {
    DynBuffer buffer;
    initDynBuffer(&buffer);
    sprintfDyn(&buffer, 0, "[");
    ListIterator it = createIterator(iList);
    bool first = true;
    for (void* data = nextElement(&it); data; data = nextElement(&it)) {
        if (!first) {
            sprintfDyn(&buffer, 0, ",");
        }
        Individual* indi = (Individual*)data;
        char* indiBuf = indToJSON(indi);
        sprintfDyn(&buffer, 0, indiBuf);
        free(indiBuf);
        first = false;
    }
    sprintfDyn(&buffer, 0, "]");
    return buffer.str;
}

char* gListToJSON(List gList) {
    DynBuffer buffer;
    initDynBuffer(&buffer);
    sprintfDyn(&buffer, 0, "[");
    ListIterator it = createIterator(gList);
    bool first = true;
    for (void* data = nextElement(&it); data; data = nextElement(&it)) {
        List* list = (List*)data;
        if (!first) {
            sprintfDyn(&buffer, 0, ",");
        }
        char* buf = iListToJSON(*list);
        sprintfDyn(&buffer, 0, buf);
        free(buf);
        first = false;
    }
    sprintfDyn(&buffer, 0, "]");
    return buffer.str;
}

void deleteGeneration(void* toBeDeleted) {
    clearList((List*)toBeDeleted);
    free(toBeDeleted);
}
int compareGenerations(const void* first,const void* second) {
    return comparePointers(first, second);
}
char* printGeneration(void* toBePrinted) {
    return toString(*(List*)toBePrinted);
}
