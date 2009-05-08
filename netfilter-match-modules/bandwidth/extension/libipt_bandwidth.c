/* 
 *
 *  Copyright © 2008 by Eric Bishop <eric@gargoyle-router.com>
 * 
 *  This file is free software: you may copy, redistribute and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation, either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This file is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

//in iptables 1.4.0 and higher, iptables.h includes xtables.h, which
//we can use to check whether we need to deal with the new requirements
//in pre-processor directives below
#include <iptables.h>  
#include <linux/netfilter_ipv4/ipt_bandwidth.h>





/* Function which prints out usage message. */
static void help(void)
{
	printf(	"bandwidth options:\n  --greater_than [BYTES]\n  --less_than [BYTES]\n  --current_Bandwidth [BYTES]\n  --reset_interval [minute|hour|day|week|month]\n");
}

static struct option opts[] = 
{
	{ .name = "--less_than", 		.has_arg = 1, .flag = 0, .val = BANDWIDTH_LT },	
	{ .name = "--greater_than", 		.has_arg = 1, .flag = 0, .val = BANDWIDTH_GT },
	{ .name = "--current_bandwidth",	.has_arg = 1, .flag = 0, .val = BANDWIDTH_CURRENT },	
	{ .name = "--reset_interval",		.has_arg = 1, .flag = 0, .val = BANDWIDTH_RESET },
	{ .name = 0 }
};


/* Function which parses command options; returns true if it
   ate an option */
static int parse(	int c, 
			char **argv,
			int invert,
			unsigned int *flags,
#ifdef _XTABLES_H
			const void *entry,
#else
			const struct ipt_entry *entry,
			unsigned int *nfcache,
#endif			
			struct ipt_entry_match **match
			)
{
	struct ipt_bandwidth_info *info = (struct ipt_bandwidth_info *)(*match)->data;
	int valid_arg = 0;
	int num_read;
	u_int64_t read;

	
	switch (c)
	{
		case BANDWIDTH_LT:
			num_read = sscanf(argv[optind-1], "%lld", &read);
			if(num_read > 0 && *flags % BANDWIDTH_CURRENT == 0)
			{
				info->gt_lt = BANDWIDTH_LT;
				info->bandwidth_cutoff = read;
				valid_arg = 1;
			}
			break;
		case BANDWIDTH_GT:
			num_read = sscanf(argv[optind-1], "%lld", &read);
			if(num_read > 0 && *flags % BANDWIDTH_CURRENT == 0)
			{
				info->gt_lt = BANDWIDTH_GT;
				info->bandwidth_cutoff = read;
				valid_arg = 1;
			}
			break;
		case BANDWIDTH_CURRENT:
			num_read = sscanf(argv[optind-1], "%lld", &read);
			if(num_read > 0 )
			{
				info->current_bandwidth = read;
				valid_arg = 1;
			}
			break;
		case BANDWIDTH_RESET:
			valid_arg = 1;
			*flags = *flags + BANDWIDTH_RESET;
			if(strcmp(argv[optind-1],"minute") ==0)
			{
				info->reset_interval = BANDWIDTH_MINUTE;
			}
			else if(strcmp(argv[optind-1],"hour") ==0)
			{
				info->reset_interval = BANDWIDTH_HOUR;
			}
			else if(strcmp(argv[optind-1],"day") ==0)
			{
				info->reset_interval = BANDWIDTH_DAY;
			}
			else if(strcmp(argv[optind-1],"week") ==0)
			{
				info->reset_interval = BANDWIDTH_WEEK;
			}
			else if(strcmp(argv[optind-1],"month") ==0)
			{
				info->reset_interval = BANDWIDTH_MONTH;
			}
			else if(strcmp(argv[optind-1],"never") ==0)
			{
				info->reset_interval = BANDWIDTH_NEVER;
			}
			else
			{
				*flags = *flags - BANDWIDTH_RESET;
				valid_arg = 0;
			}
			break;
	}
	*flags = *flags + (unsigned int)c;
	
	if(*flags % BANDWIDTH_RESET < BANDWIDTH_CURRENT)
	{
		info->current_bandwidth = 0;
	}
	if(*flags < BANDWIDTH_RESET)
	{
		info->reset_interval = BANDWIDTH_NEVER;
	}

	info->next_reset = 0;

	return valid_arg;
}


	
static void print_bandwidth_args(	struct ipt_bandwidth_info* info )
{
	if(info->gt_lt == BANDWIDTH_GT)
	{
		printf(" --greater_than %lld ", info->bandwidth_cutoff);
	}
	if(info->gt_lt == BANDWIDTH_LT)
	{
		printf(" --less_than %lld ", info->bandwidth_cutoff);
	}
	printf(" --current_bandwidth %lld ", info->current_bandwidth);
	if(info->reset_interval == BANDWIDTH_MINUTE)
	{
		printf(" --reset_interval minute ");
	}
	else if(info->reset_interval == BANDWIDTH_HOUR)
	{
		printf(" --reset_interval hour ");
	}
	else if(info->reset_interval == BANDWIDTH_DAY)
	{
		printf(" --reset_interval day ");
	}
	else if(info->reset_interval == BANDWIDTH_WEEK)
	{
		printf(" --reset_interval week ");
	}
	else if(info->reset_interval == BANDWIDTH_MONTH)
	{
		printf(" --reset_interval month ");
	}
}

/* Final check; must have specified a test string with either --contains or --contains_regex. */
static void final_check(unsigned int flags)
{
	if(flags % BANDWIDTH_CURRENT != BANDWIDTH_LT && flags % BANDWIDTH_CURRENT != BANDWIDTH_GT )
	{
		exit_error(PARAMETER_PROBLEM, "You must specify '--greater_than' or '--less_than' '");
	}
}

/* Prints out the matchinfo. */
#ifdef _XTABLES_H
static void print(const void *ip, const struct xt_entry_match *match, int numeric)
#else	
static void print(const struct ipt_ip *ip, const struct ipt_entry_match *match, int numeric)
#endif
{
	printf("bandwidth ");
	struct ipt_bandwidth_info *info = (struct ipt_bandwidth_info *)match->data;

	print_bandwidth_args(info);
}

/* Saves the union ipt_matchinfo in parsable form to stdout. */
#ifdef _XTABLES_H
static void save(const void *ip, const struct xt_entry_match *match)
#else
static void save(const struct ipt_ip *ip, const struct ipt_entry_match *match)
#endif
{
	struct ipt_bandwidth_info *info = (struct ipt_bandwidth_info *)match->data;
	print_bandwidth_args(info);
}

static struct iptables_match bandwidth = 
{ 
	.next		= NULL,
 	.name		= "bandwidth",
	.version	= IPTABLES_VERSION,
	.size		= IPT_ALIGN(sizeof(struct ipt_bandwidth_info)),
	.userspacesize	= IPT_ALIGN(sizeof(struct ipt_bandwidth_info)),
	.help		= &help,
	.parse		= &parse,
	.final_check	= &final_check,
	.print		= &print,
	.save		= &save,
	.extra_opts	= opts
};

void _init(void)
{
	register_match(&bandwidth);
}
