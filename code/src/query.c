/* Query.c
 *
 * Syntax:
 * query -[option1]=value -[option2]=value ... -[optionN]=value
 * or
 * query value
 *
 * Options:
 * -t | -type = mob | area | obj | char | mprog
 * -n | -name = search by name (C regular expressions permitted)
 * -l | -level = level or level range (-l23 or -l23:56)
 * -a | -area = area vnum, area vnum range, or precede with question mark to find area containing vnum (only one)
 * -v | -vnums = range of vnums (-v1000:5000)
 * -h | -help = show help either for whole command (no value) or for specific option.
 * -format = area | brief | full.
 *          area shows the vnum and name of the containing area and is ignored for area type search
 *          brief depends on the type of search, and is default
 *          full depends on the type of search.
 *
 *
 * Notes:
 * If no option is provided, value is assumed to represent object name.
 * Max results returned is 250.
 * One of -type, -area, or -vnums must be provided in any query except default.
 * -help=help will not result in recursive madness
 *
 * Examples:
 * query -t=mob -n=revolv
 *  find any mob with a name that matches revolv (the Revolving Drunk in the Mob Factory, for example)
 *
 * query -t=area -n=mob
 *  find an area with a name containing mob (the Mob Factory, for example)
 */
