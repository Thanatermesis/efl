#include "config.h"
#include "shader/evas_gl_shaders.x"
#include "evas_gl_common.h"

#define SHADER_FLAG_SAM_BITSHIFT 3
#define SHADER_FLAG_MASKSAM_BITSHIFT 6
#define SHADER_PROG_NAME_FMT "/shader/%08x"
#define SHADER_BINARY_EET_COMPRESS 1
#define SHADER_EET_CHECKSUM "/shader/checksum"
#define SHADER_EET_CACHENAME "binary_shader"

#define P(i) ((void*)(intptr_t)(i))
#define I(p) ((int)(intptr_t)(p))

#ifdef WORDS_BIGENDIAN
# define BASEFLAG SHADER_FLAG_DITHER | SHADER_FLAG_BIGENDIAN
#else
# define BASEFLAG SHADER_FLAG_DITHER
#endif

typedef enum {
   SHADER_FLAG_TEX               = (1 << 0),
   SHADER_FLAG_BGRA              = (1 << 1),
   SHADER_FLAG_MASK              = (1 << 2),
   SHADER_FLAG_SAM12             = (1 << (SHADER_FLAG_SAM_BITSHIFT + 0)),
   SHADER_FLAG_SAM21             = (1 << (SHADER_FLAG_SAM_BITSHIFT + 1)),
   SHADER_FLAG_SAM22             = (1 << (SHADER_FLAG_SAM_BITSHIFT + 2)),
   SHADER_FLAG_MASKSAM12         = (1 << (SHADER_FLAG_MASKSAM_BITSHIFT + 0)),
   SHADER_FLAG_MASKSAM21         = (1 << (SHADER_FLAG_MASKSAM_BITSHIFT + 1)),
   SHADER_FLAG_MASKSAM22         = (1 << (SHADER_FLAG_MASKSAM_BITSHIFT + 2)),
   SHADER_FLAG_MASK_COLOR        = (1 << 9),
   SHADER_FLAG_IMG               = (1 << 10),
   SHADER_FLAG_BIGENDIAN         = (1 << 11),
   SHADER_FLAG_YUV               = (1 << 12),
   SHADER_FLAG_YUY2              = (1 << 13),
   SHADER_FLAG_NV12              = (1 << 14),
   SHADER_FLAG_YUV_709           = (1 << 15),
   SHADER_FLAG_EXTERNAL          = (1 << 16),
   SHADER_FLAG_AFILL             = (1 << 17),
   SHADER_FLAG_NOMUL             = (1 << 18),
   SHADER_FLAG_ALPHA             = (1 << 19),
   SHADER_FLAG_RGB_A_PAIR        = (1 << 20),
   SHADER_FLAG_FILTER_DISPLACE   = (1 << 21),
   SHADER_FLAG_FILTER_CURVE      = (1 << 22),
   SHADER_FLAG_FILTER_BLUR       = (1 << 23),
   SHADER_FLAG_FILTER_DIR_Y      = (1 << 24),
   SHADER_FLAG_FILTER_ALPHA_ONLY = (1 << 25),
   SHADER_FLAG_FILTER_GRAYSCALE  = (1 << 26),
   SHADER_FLAG_FILTER_INVERSE_COLOR  = (1 << 27),
   SHADER_FLAG_DITHER            = (1 << 28),
} Shader_Flag;
#define SHADER_FLAG_COUNT 29

static const char *_shader_flags[SHADER_FLAG_COUNT] = {
   "TEX",
   "BGRA",
   "MASK",
   "SAM12",
   "SAM21",
   "SAM22",
   "MASKSAM12",
   "MASKSAM21",
   "MASKSAM22",
   "MASK_COLOR",
   "IMG",
   "BIGENDIAN",
   "YUV",
   "YUY2",
   "NV12",
   "YUV_709",
   "EXTERNAL",
   "AFILL",
   "NOMUL",
   "ALPHA",
   "RGB_A_PAIR",
   "FILTER_DISPLACE",
   "FILTER_CURVE",
   "FILTER_BLUR",
   "FILTER_DIR_Y",
   "ALPHA_ONLY",
   "FILTER_GRAYSCALE",
   "FILTER_INVERSE_COLOR",
   "DITHER",
};

static Eina_Bool compiler_released = EINA_FALSE;
static Eina_Bool _do_dither = EINA_TRUE;

/**
 * @brief Logs any error that occurs during GL shader compilation or linking.
 *
 * This function retrieves the info log from a shader or program object and
 * prints it to the error log. It is used to debug shader compilation and
 * linking issues.
 *
 * @param target The ID of the shader or program object that failed.
 * @param action A string describing the failed operation (e.g., "compile vertex shader").
 * @param is_shader EINA_TRUE if the target is a shader, EINA_FALSE for a program.
 */
static void
gl_compile_link_error(GLuint target, const char *action, Eina_Bool is_shader)
{
   int loglen = 0, chars = 0;
   char *logtxt;

   if (is_shader)
     /* Shader info log */
     glGetShaderiv(target, GL_INFO_LOG_LENGTH, &loglen);
   else
     /* Program info log */
     glGetProgramiv(target, GL_INFO_LOG_LENGTH, &loglen);

   if (loglen > 0)
     {
        logtxt = calloc(loglen, sizeof(char));
        if (logtxt)
          {
             if (is_shader) glGetShaderInfoLog(target, loglen, &chars, logtxt);
             else glGetProgramInfoLog(target, loglen, &chars, logtxt);
             ERR("Failed to %s: %s", action, logtxt);
             free(logtxt);
          }
     }
}

/**
 * @brief Binds the standard vertex attributes to predefined locations for a shader program.
 *
 * This ensures that the application C code and the GLSL shaders agree on the
 * locations for vertex data like position, color, and texture coordinates.
 *
 * @param prg The ID of the shader program.
 */
static inline void
_attributes_bind(GLint prg)
{
   glBindAttribLocation(prg, SHAD_VERTEX,  "vertex");
   glBindAttribLocation(prg, SHAD_COLOR,   "color");
   glBindAttribLocation(prg, SHAD_TEXUV,   "tex_coord");
   glBindAttribLocation(prg, SHAD_TEXUV2,  "tex_coord2");
   glBindAttribLocation(prg, SHAD_TEXUV3,  "tex_coord3");
   glBindAttribLocation(prg, SHAD_TEXA,    "tex_coorda");
   glBindAttribLocation(prg, SHAD_TEXSAM,  "tex_sample");
   glBindAttribLocation(prg, SHAD_MASK,    "mask_coord");
   glBindAttribLocation(prg, SHAD_MASKSAM, "tex_masksample");
}

/**
 * @brief Loads a pre-compiled shader program from an Eet cache file.
 *
 * This function attempts to load a shader program binary corresponding to the
 * given flags from the cache. This avoids recompiling the shader from source,
 * significantly speeding up initialization.
 *
 * The function includes a workaround for a rendering bug with glProgramBinary
 * by creating and attaching dummy shaders before loading the binary.
 *
 * @param ef The Eet file handle to the shader cache.
 * @param flags A bitmask identifying the specific shader variant to load.
 * @return A new Evas_GL_Program structure if successful, otherwise NULL.
 */
static Evas_GL_Program *
_evas_gl_common_shader_program_binary_load(Eet_File *ef, unsigned int flags)
{
   int num = 0, length = 0;
   int *formats = NULL;
   void *data = NULL;
   char pname[32];
   GLint ok = 0, prg, vtx = GL_NONE, frg = GL_NONE;
   Evas_GL_Program *p = NULL;
   Eina_Bool direct = 1;

   if (!ef || !glsym_glProgramBinary) return NULL;

   sprintf(pname, SHADER_PROG_NAME_FMT, flags);
   data = (void *) eet_read_direct(ef, pname, &length);
   if (!data)
     {
        data = eet_read(ef, pname, &length);
        direct = 0;
     }
   if ((!data) || (length <= 0)) goto finish;

   glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &num);
   if (num <= 0) goto finish;

   formats = calloc(num, sizeof(int));
   if (!formats) goto finish;

   glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, formats);
   if (!formats[0]) goto finish;

   prg = glCreateProgram();
#if 1
   // TODO: invalid rendering error occurs when attempting to use a
   // glProgramBinary. in order to render correctly we should create a dummy
   // vertex shader.
   vtx = glCreateShader(GL_VERTEX_SHADER);
   glAttachShader(prg, vtx);
   frg = glCreateShader(GL_FRAGMENT_SHADER);
   glAttachShader(prg, frg);
#endif
   glsym_glProgramBinary(prg, formats[0], data, length);

   _attributes_bind(prg);

   glGetProgramiv(prg, GL_LINK_STATUS, &ok);
   if (!ok)
     {
        gl_compile_link_error(prg, "load a program object", EINA_FALSE);
        ERR("Abort load of program (%s)", pname);
        glDeleteProgram(prg);
        goto finish;
     }

   p = calloc(1, sizeof(*p));

   GLuint curr_prog = 0;
   glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *)&curr_prog);

   p->flags = flags;
   p->prog = prg;
   p->reset = EINA_TRUE;
   p->bin_saved = EINA_TRUE;

   glUseProgram(prg);
   p->uniform.mvp = glGetUniformLocation(prg, "mvp");
   p->uniform.rotation_id = glGetUniformLocation(prg, "rotation_id");
   evas_gl_common_shader_textures_bind(p, EINA_FALSE);

   glUseProgram(curr_prog);

finish:
   if (vtx) glDeleteShader(vtx);
   if (frg) glDeleteShader(frg);
   free(formats);
   if (!direct) free(data);
   return p;
}

/**
 * @brief Saves a compiled shader program binary to an Eet cache file.
 *
 * If supported by the driver, this function retrieves the binary representation
 * of a linked shader program and stores it in the provided Eet file. This
 * allows for faster loading in subsequent runs using
 * _evas_gl_common_shader_program_binary_load().
 *
 * @param p The shader program to save.
 * @param ef The Eet file handle to the shader cache.
 * @return 1 on success, 0 on failure.
 */
static int
_evas_gl_common_shader_program_binary_save(Evas_GL_Program *p, Eet_File *ef)
{
   void* data = NULL;
   GLenum format;
   int length = 0, size = 0;
   char pname[32];

   if (!glsym_glGetProgramBinary) return 0;

   glGetProgramiv(p->prog, GL_PROGRAM_BINARY_LENGTH, &length);
   if (length <= 0) return 0;

   data = malloc(length);
   if (!data) return 0;

   glsym_glGetProgramBinary(p->prog, length, &size, &format, data);

   if (length != size)
     {
        free(data);
        return 0;
     }

   sprintf(pname, SHADER_PROG_NAME_FMT, p->flags);
   if (eet_write(ef, pname, data, length, SHADER_BINARY_EET_COMPRESS) < 0)
     {
        free(data);
        return 0;
     }

   free(data);
   p->bin_saved = 1;
   return 1;
}

/**
 * @brief Computes and caches a checksum of the core shader source files.
 *
 * This hash is used to validate the binary shader cache. If the shader source
 * code changes, the hash will change, and the cache will be invalidated,
 * forcing shaders to be recompiled.
 *
 * @param shared The shared GL data structure where the checksum is stored.
 */
static void
_evas_gl_common_shader_binary_hash(Evas_GL_Shared *shared)
{
   if (shared->shaders_checksum)
     return;

   /* This hash makes it sure that if the shaders code changes, then we
    * will not reuse the old binaries. */
   shared->shaders_checksum = eina_stringshare_printf
         ("%#x:%#x",
          eina_hash_superfast(fragment_glsl, strlen(fragment_glsl)),
          eina_hash_superfast(vertex_glsl, strlen(vertex_glsl)));
}

/**
 * @brief Verifies the checksum of the shader cache against the current shader source.
 *
 * This function compares the checksum stored in the Eet cache file with the
 * checksum of the current in-memory shader source code to ensure the cache is
 * not stale.
 *
 * @param shared The shared GL data containing the current shader checksum.
 * @param ef The Eet file handle to the shader cache.
 * @return EINA_TRUE if the checksums match, EINA_FALSE otherwise.
 */
static Eina_Bool
_evas_gl_common_shader_binary_checksum_check(Evas_GL_Shared *shared, Eet_File *ef)
{
   Eina_Bool ret = EINA_FALSE;
   const char *old_hash;
   int len = 0;

   if (!ef) return EINA_FALSE;
   _evas_gl_common_shader_binary_hash(shared);
   old_hash = eet_read_direct(ef, SHADER_EET_CHECKSUM, &len);
   if (old_hash &&
       (len == (eina_stringshare_strlen(shared->shaders_checksum) + 1)) &&
       (!strcmp(shared->shaders_checksum, old_hash)))
     ret = EINA_TRUE;

   return ret;
}

/**
 * @brief Writes the current shader source checksum to the cache file.
 *
 * This is done to mark the cache as valid for the current version of the
 * shader source code.
 *
 * @param shared The shared GL data containing the current shader checksum.
 * @param ef The Eet file handle to write the checksum to.
 * @return EINA_TRUE on successful write, EINA_FALSE otherwise.
 */
static Eina_Bool
_evas_gl_common_shader_binary_checksum_write(Evas_GL_Shared *shared, Eet_File *ef)
{
   int ret, len;

   if (!ef) return EINA_FALSE;
   _evas_gl_common_shader_binary_hash(shared);
   len = eina_stringshare_strlen(shared->shaders_checksum) + 1;
   ret = eet_write(ef, SHADER_EET_CHECKSUM, shared->shaders_checksum, len, 0);

   return (ret == len);
}

/**
 * @brief Initializes the shader binary cache system.
 *
 * This function checks if program binaries are supported, finds or creates the
 * cache directory, and opens the cache file if it exists and its checksum is
 * valid. A valid cache file is stored in `shared->shaders_cache`.
 *
 * @param shared The shared GL data structure.
 * @return 1 on success (or if caching is disabled), 0 on failure.
 */
static int
_evas_gl_common_shader_binary_init(Evas_GL_Shared *shared)
{
   Eet_File *ef = NULL;
   char bin_dir_path[PATH_MAX];
   char bin_file_path[PATH_MAX];

   if (!shared || !shared->info.bin_program)
     return 1;

   if (shared->shaders_cache)
     return 1;

   if (!evas_gl_common_file_cache_dir_check(bin_dir_path, sizeof(bin_dir_path)))
     return 0;

   if (!evas_gl_common_file_cache_file_check(bin_dir_path, SHADER_EET_CACHENAME,
                                             bin_file_path, sizeof(bin_dir_path)))
     return 0;

   if (!eet_init()) return 0;
   ef = eet_open(bin_file_path, EET_FILE_MODE_READ);
   if (!_evas_gl_common_shader_binary_checksum_check(shared, ef))
     goto error;

   shared->shaders_cache = ef;
   return 1;

error:
   if (ef) eet_close(ef);
   eet_shutdown();
   return 0;
}

/**
 * @brief Saves all newly compiled shaders to the binary cache file.
 *
 * This function iterates through all shader programs in the hash table. For
 * any program that hasn't been saved to the binary cache yet, it saves it.
 * It uses a temporary file and atomic rename to ensure cache integrity.
 *
 * @param shared The shared GL data structure.
 * @return 1 on success, 0 on failure.
 */
static int
_evas_gl_common_shader_binary_save(Evas_GL_Shared *shared)
{
   char bin_dir_path[PATH_MAX];
   char bin_file_path[PATH_MAX];
   char tmp_file_name[PATH_MAX + PATH_MAX + 128];
   int tmpfd = -1, copy;
   Eina_Tmpstr *tmp_file_path = NULL;
   Eet_File *ef = NULL, *ef0 = NULL;
   Evas_GL_Program *p;
   Eina_Iterator *it;
#ifdef _WIN32
   char *bin_file_path_2;
   char *result_backslash;
#endif

   /* use eet */
   if (!eet_init()) return 0;

   if (!evas_gl_common_file_cache_dir_check(bin_dir_path, sizeof(bin_dir_path)))
     {
        if (!evas_gl_common_file_cache_mkpath(bin_dir_path))
          return 0; /* we can't make directory */
     }

   copy = evas_gl_common_file_cache_file_check(bin_dir_path, SHADER_EET_CACHENAME,
                                               bin_file_path, sizeof(bin_dir_path));

   /* use mkstemp for writing */
#ifdef _WIN32
   /*
    * get basename so that the temporary file is created in
    * the tmp directory with eina_file_mkstemp()
    */
   if ((bin_file_path_2 = strrchr(bin_file_path, '/'))) bin_file_path_2++;
   else bin_file_path_2 = bin_file_path;
   if ((result_backslash = strrchr(bin_file_path_2, '\\')))
     bin_file_path_2 = ++result_backslash;

   snprintf(tmp_file_name, sizeof(tmp_file_name), "%s.XXXXXX.cache", bin_file_path_2);
#else
   snprintf(tmp_file_name, sizeof(tmp_file_name), "%s.XXXXXX.cache", bin_file_path);
#endif

   tmpfd = eina_file_mkstemp(tmp_file_name, &tmp_file_path);
   if (tmpfd < 0) goto error;

   /* copy old file */
   if (copy)
     {
        ef = eet_open(tmp_file_path, EET_FILE_MODE_READ);
        if (!ef) goto save;
        if (!_evas_gl_common_shader_binary_checksum_check(shared, ef))
          copy = EINA_FALSE;
        eet_close(ef);
        if (copy)
          eina_file_copy(bin_file_path, tmp_file_path, EINA_FILE_COPY_DATA, NULL, NULL);
     }

save:

   ef = eet_open(tmp_file_path, EET_FILE_MODE_WRITE);
   if (!ef) goto error;

   if (copy) ef0 = shared->shaders_cache;

   if (!_evas_gl_common_shader_binary_checksum_write(shared, ef))
     goto error;

   if (ef0)
     {
        char **keys;
        int keys_num = 0, i;

        keys = eet_list(ef0, "/shader/*", &keys_num);
        if (keys)
          {
             for (i = 0; i < keys_num; i++)
               {
                  int len = 0;
                  void *data = eet_read(ef0, keys[i], &len);
                  if ((data) && (len > 0))
                    eet_write(ef, keys[i], data, len, SHADER_BINARY_EET_COMPRESS);
                  free(data);
               }
             free(keys);
          }
     }
   it = eina_hash_iterator_data_new(shared->shaders_hash);
   EINA_ITERATOR_FOREACH(it, p)
     {
        if (!p->bin_saved)
          {
             if (_evas_gl_common_shader_program_binary_save(p, ef))
               p->bin_saved = 1;
          }
     }
   eina_iterator_free(it);

   if (shared->shaders_cache)
     {
        eet_close(shared->shaders_cache);
        shared->shaders_cache = NULL;
        eet_shutdown();
     }

   if (eet_close(ef) != EET_ERROR_NONE) goto destroyed;
#ifdef _WIN32
   /* no other choice on Windows: copy file from tmp dir to cache dir */
   if (!CopyFile(tmp_file_path, bin_file_path, FALSE))
     goto destroyed;
#else
   if (rename(tmp_file_path, bin_file_path) < 0) goto destroyed;
#endif
   eina_tmpstr_del(tmp_file_path);
   close(tmpfd);
   eet_shutdown();

   shared->needs_shaders_flush = 0;
   return 1;

 destroyed:
   ef = NULL;

 error:
   if (tmpfd >= 0) close(tmpfd);
   if (ef) eet_close(ef);
   if (evas_gl_common_file_cache_file_exists(tmp_file_path))
     unlink(tmp_file_path);
   eina_tmpstr_del(tmp_file_path);
   eet_shutdown();
   return 0;
}

/**
 * @brief Deletes a shader program and its associated resources.
 *
 * This includes freeing any filter-specific textures and the GL program object
 * itself.
 *
 * @param p The shader program to delete.
 */
static inline void
_program_del(Evas_GL_Program *p)
{
   if (p->filter)
     {
        if (p->filter->texture.tex_ids[0])
          glDeleteTextures(1, p->filter->texture.tex_ids);
        free(p->filter);
     }
   if (p->prog) glDeleteProgram(p->prog);
   free(p);
}

/**
 * @brief Callback function used by eina_hash to free a shader program.
 *
 * @param data A pointer to the Evas_GL_Program to be deleted.
 */
static void
_shaders_hash_free_cb(void *data)
{
   _program_del(data);
}

/**
 * @brief Generates a full GLSL shader source string from a base template and feature flags.
 *
 * This function prepends a series of `#define` directives to the base GLSL code
 * based on the provided flags. This allows for conditional compilation within
 * the shader to enable or disable features. The `EVAS_GL_SHADER_GLSL_VERSION`
 * environment variable can be used to override the GLSL version for debugging.
 *
 * @param flags A bitmask of Shader_Flag values that control which `#define`s are added.
 * @param base The base GLSL source code template.
 * @return A newly allocated string containing the complete shader source. The
 *         caller is responsible for freeing this string.
 */
static char *
evas_gl_common_shader_glsl_get(unsigned int flags, const char *base)
{
   Eina_Strbuf *s = eina_strbuf_new();
   unsigned int k;
   char *str;

   /* This is an env var to use for debugging purposes only */
   static const char *evas_gl_shader_glsl_version = NULL;
   if (!evas_gl_shader_glsl_version)
     {
        evas_gl_shader_glsl_version = getenv("EVAS_GL_SHADER_GLSL_VERSION");
        if (!evas_gl_shader_glsl_version) evas_gl_shader_glsl_version = "";
        else WRN("Using GLSL version tag: '%s'", evas_gl_shader_glsl_version);
     }

   if (*evas_gl_shader_glsl_version)
     eina_strbuf_append_printf(s, "#version %s\n", evas_gl_shader_glsl_version);

   for (k = 0; k < SHADER_FLAG_COUNT; k++)
     {
        if (flags & (1 << k))
          eina_strbuf_append_printf(s, "#define SHD_%s\n", _shader_flags[k]);
     }

   eina_strbuf_append(s, base);
   str = eina_strbuf_string_steal(s);
   eina_strbuf_free(s);
   return str;
}

/**
 * @brief Compiles and links a vertex and fragment shader into a GL program.
 *
 * This function handles the standard OpenGL process of creating shader objects,
 * compiling source, creating a program object, attaching shaders, and linking
 * them. It also sets the `GL_PROGRAM_BINARY_RETRIEVABLE_HINT` to enable saving
 * the compiled binary later.
 *
 * @param flags The feature flags associated with this shader program.
 * @param vertex The null-terminated string containing the vertex shader source code.
 * @param fragment The null-terminated string containing the fragment shader source code.
 * @return A new Evas_GL_Program on success, or NULL on failure.
 */
static Evas_GL_Program *
evas_gl_common_shader_compile(unsigned int flags, const char *vertex,
                              const char *fragment)
{
   Evas_GL_Program *p;
   GLuint vtx, frg, prg;
   GLint ok = 0;

   compiler_released = EINA_FALSE;
   vtx = glCreateShader(GL_VERTEX_SHADER);
   frg = glCreateShader(GL_FRAGMENT_SHADER);

   glShaderSource(vtx, 1, &vertex, NULL);
   glCompileShader(vtx);
   glGetShaderiv(vtx, GL_COMPILE_STATUS, &ok);
   if (!ok)
     {
        gl_compile_link_error(vtx, "compile vertex shader", EINA_TRUE);
        ERR("Abort compile of vertex shader:\n%s", vertex);
        glDeleteShader(vtx);
        return NULL;
     }
   ok = 0;

   glShaderSource(frg, 1, &fragment, NULL);
   glCompileShader(frg);
   glGetShaderiv(frg, GL_COMPILE_STATUS, &ok);
   if (!ok)
     {
        gl_compile_link_error(frg, "compile fragment shader", EINA_TRUE);
        ERR("Abort compile of fragment shader:\n%s", fragment);
        glDeleteShader(vtx);
        glDeleteShader(frg);
        return NULL;
     }
   ok = 0;

   prg = glCreateProgram();
#ifndef GL_GLES
   if ((glsym_glGetProgramBinary) && (glsym_glProgramParameteri))
     glsym_glProgramParameteri(prg, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
#endif
   glAttachShader(prg, vtx);
   glAttachShader(prg, frg);

   _attributes_bind(prg);

   glLinkProgram(prg);
   glGetProgramiv(prg, GL_LINK_STATUS, &ok);
   if (!ok)
     {
        gl_compile_link_error(prg, "link fragment and vertex shaders", EINA_FALSE);
        ERR("Abort compile of shader (flags: %08x)", flags);
        glDeleteShader(vtx);
        glDeleteShader(frg);
        glDeleteProgram(prg);
        return 0;
     }

   p = calloc(1, sizeof(*p));
   p->flags = flags;
   p->prog = prg;
   p->reset = EINA_TRUE;

   glDeleteShader(vtx);
   glDeleteShader(frg);

   return p;
}

/**
 * @brief Generates a list of common shader flag combinations to be precompiled.
 *
 * To avoid jank during runtime, Evas pre-compiles a set of frequently used
 * shaders at startup. This function generates the list of shader variants
 * (represented by their flags) that cover common cases like simple rectangles,
 * text, and images with various options.
 *
 * @param shared The shared GL data.
 * @return An Eina_List containing integer pointers, where each integer is a
 *         shader flag combination. The caller is responsible for freeing the list.
 */
static Eina_List *
evas_gl_common_shader_precompile_list(Evas_GL_Shared *shared)
{
   int bgra, mask, sam, masksam, img, nomul, afill, yuv;
   Eina_List *li = NULL;
   unsigned int baseflags = 0;

   if (!shared) return NULL;
   if (_do_dither) baseflags |= BASEFLAG;
   // rect
   li = eina_list_append(li, P(baseflags));

   // text
   for (mask = 0; mask <= 1; mask++)
     for (masksam = SHD_SAM11; masksam < (mask ? SHD_SAM_LAST : 1); masksam++)
       {
          int           flags  = baseflags | SHADER_FLAG_TEX | SHADER_FLAG_ALPHA;
          if (mask)     flags |= SHADER_FLAG_MASK;
          if (masksam)  flags |= (1 << (SHADER_FLAG_MASKSAM_BITSHIFT + masksam - 1));
          li = eina_list_append(li, P(flags));
       }

   // images
   for (mask = 0; mask <= 1; mask++)
     for (masksam = SHD_SAM11; masksam < (mask ? SHD_SAM_LAST : 1); masksam++)
       for (sam = SHD_SAM11; sam < SHD_SAM_LAST; sam++)
         for (bgra = 0; bgra <= shared->info.bgra; bgra++)
           for (img = 0; img <= 1; img++)
             for (nomul = 0; nomul <= 1; nomul++)
               for (afill = 0; afill <= (mask ? 0 : 1); afill++)
                 {
                    int           flags  = baseflags | SHADER_FLAG_TEX;
                    if (mask)     flags |= SHADER_FLAG_MASK;
                    if (masksam)  flags |= (1 << (SHADER_FLAG_MASKSAM_BITSHIFT + masksam - 1));
                    if (sam)      flags |= (1 << (SHADER_FLAG_SAM_BITSHIFT + sam - 1));
                    if (bgra)     flags |= SHADER_FLAG_BGRA;
                    if (img)      flags |= SHADER_FLAG_IMG;
                    if (nomul)    flags |= SHADER_FLAG_NOMUL;
                    if (afill)    flags |= SHADER_FLAG_AFILL;
                    li = eina_list_append(li, P(flags));
                 }

   // yuv
   for (yuv = SHADER_FLAG_YUV; yuv <= SHADER_FLAG_YUV_709; yuv *= 2)
     for (mask = 0; mask <= 1; mask++)
       for (masksam = SHD_SAM11; masksam < (mask ? SHD_SAM_LAST : 1); masksam++)
         for (nomul = 0; nomul <= 1; nomul++)
           {
              int           flags  = baseflags | SHADER_FLAG_TEX | yuv;
              if (mask)     flags |= SHADER_FLAG_MASK;
              if (masksam)  flags |= (1 << (SHADER_FLAG_MASKSAM_BITSHIFT + masksam - 1));
              if (yuv == SHADER_FLAG_YUV_709) flags |= SHADER_FLAG_YUV;
              if (nomul)    flags |= SHADER_FLAG_NOMUL;
              li = eina_list_append(li, P(flags));
           }

   // rgb+a pair, external, and others will not be precompiled.

   DBG("Built list of %d shaders to precompile", eina_list_count(li));
   return li;
}

/**
 * @brief Generates GLSL source for given flags, compiles it, and adds it to the cache.
 *
 * This is a convenience function that orchestrates the process of turning a set
 * of shader flags into a usable Evas_GL_Program. It checks if the program
 * already exists, generates the GLSL source, compiles and links it, and upon
 * success, adds the new program to the in-memory hash of shaders.
 *
 * @param shared The shared GL data structure.
 * @param flags The bitmask of shader flags for the program to be generated.
 * @return The new Evas_GL_Program, or NULL if it already existed or compilation failed.
 */
static Evas_GL_Program *
evas_gl_common_shader_generate_and_compile(Evas_GL_Shared *shared, unsigned int flags)
{
   char *vertex, *fragment;
   Evas_GL_Program *p;

   if (eina_hash_find(shared->shaders_hash, &flags))
     return NULL;

   vertex = evas_gl_common_shader_glsl_get(flags, vertex_glsl);
   fragment = evas_gl_common_shader_glsl_get(flags, fragment_glsl);

   p = evas_gl_common_shader_compile(flags, vertex, fragment);
   if (p)
     {
        shared->needs_shaders_flush = 1;
        p->uniform.mvp = glGetUniformLocation(p->prog, "mvp");
        p->uniform.rotation_id = glGetUniformLocation(p->prog, "rotation_id");
        evas_gl_common_shader_textures_bind(p, EINA_TRUE);
        eina_hash_add(shared->shaders_hash, &flags, p);
     }
   else WRN("Failed to compile a shader (flags: %08x)", flags);

   free(vertex);
   free(fragment);

   return p;
}

/**
 * @brief Precompiles a set of common shaders.
 *
 * This function gets a list of shader variations from
 * `evas_gl_common_shader_precompile_list` and compiles each one. These
 * precompiled shaders are marked as temporary (`delete_me`), but they are
 * available immediately if needed, preventing compilation stalls during
 * rendering. This is typically done when a binary cache is not available.
 *
 * @param shared The shared GL data structure.
 * @return The number of shaders that were successfully precompiled.
 */
static int
evas_gl_common_shader_precompile_all(Evas_GL_Shared *shared)
{
   Eina_List *li = evas_gl_common_shader_precompile_list(shared);
   Evas_GL_Program *p;
   int total, cnt = 0;
   void *data;

   total = eina_list_count(li);
   EINA_LIST_FREE(li, data)
     {
        p = evas_gl_common_shader_generate_and_compile(shared, I(data));
        if (p)
          {
             p->delete_me = 1;
             cnt++;
          }
     }

   DBG("Precompiled %d/%d shaders!", cnt, total);
   return cnt;
}

/**
 * @brief Initializes the shader subsystem.
 *
 * This function sets up the shader hash table and attempts to initialize the
 * binary shader cache. It then ensures that a few of the most common shaders
 * (for rectangles, text, images) are available, either by loading them from the
 * cache or by compiling them on the fly.
 *
 * @param shared The shared GL data structure.
 * @return 1 on success.
 */
int
evas_gl_common_shader_program_init(Evas_GL_Shared *shared)
{
   /* most popular shaders */
   const int BGRA = (shared->info.bgra ? SHADER_FLAG_BGRA : 0);
   unsigned int autoload[] = {
      /* rect */ 0,
      /* text */ 0 | SHADER_FLAG_TEX | SHADER_FLAG_ALPHA,
      /* img1 */ 0 | SHADER_FLAG_TEX | SHADER_FLAG_IMG | BGRA,
      /* img2 */ 0 | SHADER_FLAG_TEX | SHADER_FLAG_IMG | SHADER_FLAG_NOMUL | BGRA,
   };
   Evas_GL_Program *p;
   unsigned i;

   if (getenv("EVAS_GL_RENDER_DISABLE_DITHER"))
     _do_dither = EINA_FALSE;

   if (_do_dither)
     {
        autoload[0] |= BASEFLAG;
        autoload[1] |= BASEFLAG;
        autoload[2] |= BASEFLAG;
        autoload[3] |= BASEFLAG;
     }
   shared->shaders_hash = eina_hash_int32_new(_shaders_hash_free_cb);
   if (_evas_gl_common_shader_binary_init(shared))
     {
        for (i = 0; i < (sizeof(autoload) / sizeof(autoload[0])); i++)
          {
             p = _evas_gl_common_shader_program_binary_load(shared->shaders_cache, autoload[i]);
             if (p)
               {
                  evas_gl_common_shader_textures_bind(p, EINA_TRUE);
                  eina_hash_add(shared->shaders_hash, &autoload[i], p);
               }
          }
     }
   else
     {
        evas_gl_common_shader_precompile_all(shared);
        for (i = 0; i < (sizeof(autoload) / sizeof(autoload[0])); i++)
          {
             p = eina_hash_find(shared->shaders_hash, &autoload[i]);
             if (p) p->delete_me = 0;
          }
        evas_gl_common_shaders_flush(shared);
     }

   return 1;
}

/**
 * @brief Flushes pending shader operations.
 * @param shared The shared GL data structure.
 *
 * This function performs two main tasks. First, it releases the shader
 * compiler if it hasn't been already, which can free up significant resources.
 * Second, if new shaders have been compiled (`needs_shaders_flush` is set),
 * it saves the entire shader cache to disk and cleans up any temporary shaders
 * that were created during precompilation but were never used.
 */
EMODAPI void
evas_gl_common_shaders_flush(Evas_GL_Shared *shared)
{

   if (!shared) return;
   if (!compiler_released)
     {
        compiler_released = EINA_TRUE;
#ifdef GL_GLES
        glReleaseShaderCompiler();
#else
        if (glsym_glReleaseShaderCompiler)
          glsym_glReleaseShaderCompiler();
#endif
     }
   if (shared->needs_shaders_flush)
     {
        Eina_List *to_delete = NULL;
        Eina_Iterator *it;
        Evas_GL_Program *p;

        _evas_gl_common_shader_binary_save(shared);

        it = eina_hash_iterator_data_new(shared->shaders_hash);
        EINA_ITERATOR_FOREACH(it, p)
          {
             if (p->delete_me)
               to_delete = eina_list_append(to_delete, p);
          }

        eina_iterator_free(it);
        EINA_LIST_FREE(to_delete, p)
          eina_hash_del(shared->shaders_hash, &p->flags, p);
     }
}

/**
 * @brief Shuts down the shader subsystem.
 *
 * Flushes any pending shader cache writes to disk, closes the cache file,
 * and frees all compiled shader programs and associated data structures.
 *
 * @param shared The shared GL data structure.
 */
void
evas_gl_common_shader_program_shutdown(Evas_GL_Shared *shared)
{
   if (!shared) return;

   if (shared->needs_shaders_flush)
     evas_gl_common_shaders_flush(shared);

   if (shared->shaders_cache)
     {
        eet_close(shared->shaders_cache);
        shared->shaders_cache = NULL;
        eet_shutdown();
     }

   eina_hash_free(shared->shaders_hash);
   shared->shaders_hash = NULL;
}

/**
 * @brief Determines the required shader feature flags for a given drawing operation.
 *
 * This function is central to Evas's shader system. It analyzes all the
 * parameters of a drawing operation (such as object type, textures, colors,
 * and blending modes) and constructs a bitmask of flags that uniquely
 * identifies the shader program needed to perform that operation.
 *
 * @param[in] shared The shared GL data.
 * @param[in] type The type of primitive being drawn (e.g., rectangle, image, text).
 * @param[in] map_points Array of map points for gradient-like effects.
 * @param[in] npoints Number of map points.
 * @param[in] r,g,b,a The multiplication color.
 * @param[in] sw,sh Source image dimensions (for an image object).
 * @param[in] w,h Destination object dimensions on canvas.
 * @param[in] smooth Whether to use smooth scaling (enables anti-aliasing).
 * @param[in] tex The primary texture.
 * @param[in] tex_only Indicates if the texture format is the only consideration.
 * @param[in] mtex The mask texture.
 * @param[in] mask_smooth Whether to use smooth scaling for the mask.
 * @param[in] mask_color Whether the mask is a color mask.
 * @param[in] mw,mh Destination mask dimensions on canvas.
 * @param[in] alphaonly For filters, indicates if only the alpha channel is affected.
 * @param[out] psam Stores the calculated sampling mode for the primary texture.
 * @param[out] pnomul Stores whether color multiplication can be skipped.
 * @param[out] pmasksam Stores the calculated sampling mode for the mask texture.
 *
 * @return An unsigned integer bitmask of Shader_Flag values.
 */
static inline unsigned int
evas_gl_common_shader_flags_get(Evas_GL_Shared *shared, Shader_Type type,
                                RGBA_Map_Point *map_points, int npoints,
                                int r, int g, int b, int a,
                                int sw, int sh, int w, int h, Eina_Bool smooth,
                                Evas_GL_Texture *tex, Eina_Bool tex_only,
                                Evas_GL_Texture *mtex, Eina_Bool mask_smooth,
                                Eina_Bool mask_color, int mw, int mh,
                                Eina_Bool alphaonly,
                                Shader_Sampling *psam, int *pnomul, Shader_Sampling *pmasksam)
{
   Shader_Sampling sam = SHD_SAM11, masksam = SHD_SAM11;
   int nomul = 1, bgra = 0, k;
   unsigned int flags = 0;

   if (_do_dither) flags |= BASEFLAG;
   // image downscale sampling
   if (smooth && ((type == SHD_IMAGE) || (type == SHD_IMAGENATIVE)))
     {
        if ((sw >= (w * 2)) && (sh >= (h * 2)))
          sam = SHD_SAM22;
        else if (sw >= (w * 2))
          sam = SHD_SAM21;
        else if (sh >= (h * 2))
          sam = SHD_SAM12;
        if (sam)
          flags |= (1 << (SHADER_FLAG_SAM_BITSHIFT + sam - 1));
     }

   // mask downscale sampling
   if (mtex && mask_smooth)
     {
        if ((mtex->w >= (mw * 2)) && (mtex->h >= (mh * 2)))
          masksam = SHD_SAM22;
        else if (mtex->w >= (mw * 2))
          masksam = SHD_SAM21;
        else if (mtex->h >= (mh * 2))
          masksam = SHD_SAM12;
        if (masksam)
          flags |= (1 << (SHADER_FLAG_MASKSAM_BITSHIFT + masksam - 1));
     }

   // mask color mode
   if (mtex && mask_color)
     {
        flags |= SHADER_FLAG_MASK_COLOR;
     }

   switch (type)
     {
      case SHD_RECT:
      case SHD_LINE:
        goto end;
      case SHD_FONT:
        flags |= (SHADER_FLAG_ALPHA | SHADER_FLAG_TEX);
        goto end;
      case SHD_IMAGE:
        flags |= SHADER_FLAG_IMG;
        break;
      case SHD_IMAGENATIVE:
        break;
      case SHD_YUV:
        flags |= SHADER_FLAG_YUV;
        break;
      case SHD_YUY2:
        flags |= SHADER_FLAG_YUY2;
        break;
      case SHD_NV12:
        flags |= SHADER_FLAG_NV12;
        break;
      case SHD_YUV_709:
        flags |= (SHADER_FLAG_YUV_709 | SHADER_FLAG_YUV);
        break;
      case SHD_RGB_A_PAIR:
      case SHD_MAP:
        break;
      case SHD_FILTER_DISPLACE:
        flags |= SHADER_FLAG_FILTER_DISPLACE;
        break;
      case SHD_FILTER_CURVE:
        flags |= SHADER_FLAG_FILTER_CURVE;
        break;
      case SHD_FILTER_BLUR_X:
        flags |= SHADER_FLAG_FILTER_BLUR;
        break;
      case SHD_FILTER_BLUR_Y:
        flags |= SHADER_FLAG_FILTER_BLUR;
        flags |= SHADER_FLAG_FILTER_DIR_Y;
        break;
      case SHD_FILTER_GRAYSCALE:
        flags |= SHADER_FLAG_FILTER_GRAYSCALE;
        break;
      case SHD_FILTER_INVERSE_COLOR:
        flags |= SHADER_FLAG_FILTER_INVERSE_COLOR;
        break;
      default:
        CRI("Impossible shader type.");
        return 0;
     }

   if (alphaonly)
     flags |= SHADER_FLAG_FILTER_ALPHA_ONLY;

   // color mul
   if ((a == 255) && (r == 255) && (g == 255) && (b == 255))
     {
        if (map_points)
          {
             for (k = 0; k < npoints; k++)
               if (map_points[k].col != 0xffffffff)
                 {
                    nomul = 0;
                    break;
                 }
          }
     }
   else
     nomul = 0;

   if (nomul)
     flags |= SHADER_FLAG_NOMUL;

   // bgra
   if (tex_only)
     {
        if (tex->im && tex->im->native.target == GL_TEXTURE_EXTERNAL_OES)
          flags |= SHADER_FLAG_EXTERNAL;
        else
          bgra = 1;
     }
   else
     bgra = shared->info.bgra;

   if (tex)
     {
        flags |= SHADER_FLAG_TEX;
        if (!tex->alpha && tex_only)
          {
             if ((flags & SHADER_FLAG_EXTERNAL) || tex->pt->dyn.img)
               flags |= SHADER_FLAG_AFILL;
          }
     }

   if (bgra)
     flags |= SHADER_FLAG_BGRA;

end:
   if (mtex)
     flags |= SHADER_FLAG_MASK;

   if (psam) *psam = sam;
   if (pnomul) *pnomul = nomul;
   if (pmasksam) *pmasksam = masksam;
   return flags;
}

/**
 * @brief Binds texture samplers to texture units for a given shader program.
 *
 * After a shader is compiled, this function must be called to set the sampler
 * uniforms (e.g., "tex", "texm") to their corresponding texture image units
 * (0, 1, 2, ...). It inspects the program's flags to determine which samplers
 * are active and assigns them consecutive texture units.
 *
 * @param p The shader program.
 * @param prog_recover If EINA_TRUE, the function will restore the previously
 *                     active GL program after it's done. This is important
 *                     when binding textures for a non-active program.
 */
void
evas_gl_common_shader_textures_bind(Evas_GL_Program *p, Eina_Bool prog_recover)
{
   struct {
      const char *name;
      int enabled;
   } textures[] = {
      { "tex", 0 },
      { "texm", 0 },
      { "texa", 0 },
      { "texu", 0 },
      { "texv", 0 },
      { "texuv", 0 },
      { "tex_filter", 0 },
      { NULL, 0 }
   };
   Eina_Bool hastex = 0;
   GLint loc;
   int i;

   if (!p || (p->tex_count > 0)) return;

   if ((p->flags & SHADER_FLAG_TEX) != 0)
     {
        textures[0].enabled = 1;
        hastex = 1;
     }
   if ((p->flags & SHADER_FLAG_MASK) != 0)
     {
        textures[1].enabled = 1;
        hastex = 1;
     }
   if ((p->flags & SHADER_FLAG_RGB_A_PAIR) != 0)
     {
        textures[2].enabled = 1;
        hastex = 1;
     }
   if (p->flags & SHADER_FLAG_YUV)
     {
        textures[3].enabled = 1;
        textures[4].enabled = 1;
        hastex = 1;
     }
   else if ((p->flags & SHADER_FLAG_NV12) || (p->flags & SHADER_FLAG_YUY2))
     {
        textures[5].enabled = 1;
        hastex = 1;
     }
   if ((p->flags & SHADER_FLAG_FILTER_DISPLACE) ||
       (p->flags & SHADER_FLAG_FILTER_CURVE) ||
       (p->flags & SHADER_FLAG_FILTER_BLUR))
     {
        textures[6].enabled = 1;
        hastex = 1;
     }

   if (hastex)
     {
        GLuint curr_prog = 0;
        if (prog_recover) glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *)&curr_prog);

        glUseProgram(p->prog); // is this necessary??
        for (i = 0; textures[i].name; i++)
          {
             if (!textures[i].enabled) continue;
             loc = glGetUniformLocation(p->prog, textures[i].name);
             if (loc < 0)
               {
                  ERR("Couldn't find uniform '%s' (shader: %08x)",
                      textures[i].name, p->flags);
               }
             glUniform1i(loc, p->tex_count++);
          }
        if (prog_recover) glUseProgram(curr_prog);
     }
}

/**
 * @brief Retrieves or creates a shader program for a specific drawing operation.
 *
 * This is the main function for obtaining a shader. It first determines the
 * necessary shader flags using `evas_gl_common_shader_flags_get()`. It then
 * looks for a matching program in the in-memory hash. If not found, it tries
 * to load it from the binary cache. As a last resort, it compiles the program
 * from source.
 *
 * The parameters are identical to evas_gl_common_shader_flags_get().
 *
 * @return A valid Evas_GL_Program for the requested drawing operation, or NULL
 *         on failure.
 */
Evas_GL_Program *
evas_gl_common_shader_program_get(Evas_Engine_GL_Context *gc,
                                  Shader_Type type,
                                  RGBA_Map_Point *map_points, int npoints,
                                  int r, int g, int b, int a,
                                  int sw, int sh, int w, int h, Eina_Bool smooth,
                                  Evas_GL_Texture *tex, Eina_Bool tex_only,
                                  Evas_GL_Texture *mtex, Eina_Bool mask_smooth,
                                  Eina_Bool mask_color, int mw, int mh,
                                  Eina_Bool alphaonly,
                                  Shader_Sampling *psam, int *pnomul,
                                  Shader_Sampling *pmasksam)
{
   unsigned int flags;
   Evas_GL_Program *p;

   flags = evas_gl_common_shader_flags_get(gc->shared, type, map_points, npoints, r, g, b, a,
                                           sw, sh, w, h, smooth, tex, tex_only,
                                           mtex, mask_smooth, mask_color, mw, mh,
                                           alphaonly, psam, pnomul, pmasksam);
   p = eina_hash_find(gc->shared->shaders_hash, &flags);
   if (!p)
     {
        _evas_gl_common_shader_binary_init(gc->shared);
        if (gc->shared->shaders_cache)
          {
             char pname[32];
             sprintf(pname, SHADER_PROG_NAME_FMT, flags);
             p = _evas_gl_common_shader_program_binary_load(gc->shared->shaders_cache, flags);
             if (p)
               {
                  evas_gl_common_shader_textures_bind(p, EINA_TRUE);
                  eina_hash_add(gc->shared->shaders_hash, &flags, p);
                  goto end;
               }
          }
        p = evas_gl_common_shader_generate_and_compile(gc->shared, flags);
        if (!p) return NULL;
     }
end:
   if (p->hitcount < PROGRAM_HITCOUNT_MAX)
     p->hitcount++;
   return p;
}
