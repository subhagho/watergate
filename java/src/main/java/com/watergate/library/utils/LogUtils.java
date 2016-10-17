/*
 *
 *  * Copyright 2014 Subhabrata Ghosh
 *  *
 *  * Licensed under the Apache License, Version 2.0 (the "License");
 *  * you may not use this file except in compliance with the License.
 *  * You may obtain a copy of the License at
 *  *
 *  *     http://www.apache.org/licenses/LICENSE-2.0
 *  *
 *  * Unless required by applicable law or agreed to in writing, software
 *  * distributed under the License is distributed on an "AS IS" BASIS,
 *  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  * See the License for the specific language governing permissions and
 *  * limitations under the License.
 *
 */

package com.watergate.library.utils;

import org.joda.time.DateTime;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Helper class to log messages.
 *
 * @author Subho Ghosh (subho dot ghosh at outlook.com)
 * @created 06/08/14
 */
public class LogUtils {
	private static final Logger log = LoggerFactory.getLogger(LogUtils.class);

	/**
	 * Print a debug message to the log.
	 *
	 * @param cls
	 *            - Calling class.
	 * @param mesg
	 *            - Message.
	 */
	public static void debug(Class<?> cls, String mesg) {
		if (log != null) {
			log.debug(format(cls, mesg));
		} else {
			System.out.println(print(cls, "DEBUG", mesg));
		}
	}

	/**
	 * Print a debug message to the log.
	 *
	 * @param cls
	 *            - Calling class.
	 * @param mesg
	 *            - Message.
	 * @param log
	 *            - Logger handle.
	 */
	public static void debug(Class<?> cls, String mesg, Logger log) {
		if (log != null) {
			log.debug(format(cls, mesg));
		} else {
			System.out.println(print(cls, "DEBUG", mesg));
		}
	}

	/**
	 * Print a debug getError to the log.
	 *
	 * @param cls
	 *            - Calling class.
	 * @param t
	 *            - Exception
	 * @param log
	 *            - Logger handle.
	 */
	public static void debug(Class<?> cls, Throwable t, Logger log) {
		if (log != null) {
			log.debug(format(cls, t.getLocalizedMessage()));
		} else {
			System.out.println(print(cls, "DEBUG", t.getLocalizedMessage()));
		}
	}

	/**
	 * Print a debug getError to the log.
	 *
	 * @param cls
	 *            - Calling class.
	 * @param t
	 *            - Exception
	 */
	public static void debug(Class<?> cls, Throwable t) {
		if (log != null) {
			log.debug(format(cls, t.getLocalizedMessage()));
		} else {
			System.out.println(print(cls, "DEBUG", t.getLocalizedMessage()));
		}
	}

	/**
	 * Print a error message to the log.
	 *
	 * @param cls
	 *            - Calling class.
	 * @param mesg
	 *            - Message.
	 * @param log
	 *            - Logger handle
	 */
	public static void error(Class<?> cls, String mesg, Logger log) {
		if (log != null) {
			log.error(format(cls, mesg));
		} else {
			System.err.println(print(cls, "ERROR", mesg));
		}
	}

	/**
	 * Print a error message to the log.
	 *
	 * @param cls
	 *            - Calling class.
	 * @param mesg
	 *            - Message.
	 */
	public static void error(Class<?> cls, String mesg) {
		if (log != null) {
			log.error(format(cls, mesg));
		} else {
			System.err.println(print(cls, "ERROR", mesg));
		}
	}

	/**
	 * Print a warning message to the log.
	 *
	 * @param cls
	 *            - Calling class.
	 * @param mesg
	 *            - Message.
	 * @param log
	 *            - Logger handle
	 */
	public static void warn(Class<?> cls, String mesg, Logger log) {
		if (log != null) {
			log.warn(format(cls, mesg));
		} else {
			System.err.println(print(cls, "WARN", mesg));
		}
	}

	/**
	 * Print a warning message to the log.
	 *
	 * @param cls
	 *            - Calling class.
	 * @param mesg
	 *            - Message.
	 */
	public static void warn(Class<?> cls, String mesg) {
		if (log != null) {
			log.warn(format(cls, mesg));
		} else {
			System.err.println(print(cls, "WARN", mesg));
		}
	}

	/**
	 * Print a getError to the log.
	 *
	 * @param cls
	 *            - Calling class.
	 * @param e
	 *            - Exception.
	 * @param log
	 *            - Logger handle
	 */
	public static void error(Class<?> cls, Throwable e, Logger log) {
		if (log != null) {
			log.error(format(cls, e.getLocalizedMessage()));
		} else {
			System.err.println(print(cls, "ERROR", e.getLocalizedMessage()));
		}
	}

	/**
	 * Print a getError to the log.
	 *
	 * @param cls
	 *            - Calling class.
	 * @param e
	 *            - Exception.
	 */
	public static void error(Class<?> cls, Throwable e) {
		if (log != null) {
			log.error(format(cls, e.getLocalizedMessage()));
		} else {
			System.err.println(print(cls, "ERROR", e.getLocalizedMessage()));
		}
	}

	/**
	 * Print a message to the log.
	 *
	 * @param cls
	 *            - Calling class.
	 * @param mesg
	 *            - Message.
	 * @param log
	 *            - Logger handle
	 */
	public static void mesg(Class<?> cls, String mesg, Logger log) {
		if (log != null) {
			log.info(format(cls, mesg));
		} else {
			System.out.println(print(cls, "INFO", mesg));
		}
	}

	/**
	 * Print a message to the log.
	 *
	 * @param cls
	 *            - Calling class.
	 * @param mesg
	 *            - Message.
	 */
	public static void mesg(Class<?> cls, String mesg) {
		if (log != null) {
			log.info(format(cls, mesg));
		} else {
			System.out.println(print(cls, "INFO", mesg));
		}
	}

	/**
	 * Recursively print the getError stacktrace to the log.
	 *
	 * @param cls
	 *            - Calling class.
	 * @param e
	 *            - Exception.
	 * @param log
	 *            - Logger handle.
	 */
	public static void stacktrace(Class<?> cls, Throwable e, Logger log) {
		if (!log.isDebugEnabled())
			return;

		debug(cls, e.getLocalizedMessage());
		StackTraceElement[] stes = e.getStackTrace();
		if (stes != null && stes.length > 0) {
			for (StackTraceElement st : stes) {
				if (log != null)
					debug(cls, format(st), log);
				else
					debug(cls, print(st));
			}
		}
		if (e.getCause() != null) {
			stacktrace(cls, e.getCause(), log);
		}
	}

	/**
	 * Recursively print the getError stacktrace to the log.
	 *
	 * @param cls
	 *            - Calling class.
	 * @param e
	 *            - Exception.
	 */
	public static void stacktrace(Class<?> cls, Throwable e) {
		if (!log.isDebugEnabled())
			return;

		debug(cls, e.getLocalizedMessage());
		StackTraceElement[] stes = e.getStackTrace();
		if (stes != null && stes.length > 0) {
			for (StackTraceElement st : stes) {
				if (log != null)
					debug(cls, format(st), log);
				else
					debug(cls, print(st));
			}
		}
		if (e.getCause() != null) {
			stacktrace(cls, e.getCause(), log);
		}
	}

	/**
	 * Print the current thread stack.
	 *
	 * @param cls
	 *            - Calling class.
	 * @param log
	 *            - Logger handle.
	 */
	public static void stack(Class<?> cls, Logger log) {
		if (!log.isDebugEnabled())
			return;

		StackTraceElement[] ste = Thread.currentThread().getStackTrace();
		if (ste != null) {
			for (StackTraceElement st : ste) {
				if (log != null)
					debug(cls, format(st), log);
				else
					debug(cls, print(st));
			}
		}
	}

	/**
	 * Print the current thread stack.
	 *
	 * @param cls
	 *            - Calling class.
	 */
	public static void stack(Class<?> cls) {
		if (!log.isDebugEnabled())
			return;

		stack(cls, log);
	}

	public static boolean isDebugEnabled() {
		return log.isDebugEnabled();
	}

	private static String format(StackTraceElement ste) {
		return String.format("%s.%s()[%d]", ste.getClassName(),
				ste.getMethodName(), ste.getLineNumber());
	}

	private static String format(Class<?> cls, String mesg) {
		return String.format("\t%s\t%s", cls.getCanonicalName(), mesg);
	}

	private static String print(Class<?> cls, String mesg, String type) {
		return String.format("%s:%s\t%s",
				new DateTime().toString("yyyy-MM-dd:HH.mm.ss.SSS"), type,
				format(cls, mesg));
	}

	private static String print(StackTraceElement ste) {
		return String
				.format("%s:DEBUG\t%s",
						new DateTime().toString("yyyy-MM-dd:HH.mm.ss.SSS"),
						format(ste));
	}
}
