#include "util/tag-sorter.h"

/*
 * Exchange two sort-entries
 * (should probably be coded inline
 * for speed increase)
 */
static void swap(tag_type *a, tag_type *b)
{
    tag_type temp;

    temp = *a;
    *a = *b;
    *b = temp;
}

/*
 * Insertion-Sort algorithm
 * (used by the Quicksort algorithm)
 */
static void insertion_sort(tag_type elements[], int number)
{
    tag_type tmp;
    for (int i = 1; i < number; i++) {
        tmp = elements[i];
        int j;
        for (j = i; (j > 0) && (elements[j - 1].tag > tmp.tag); j--)
            elements[j] = elements[j - 1];
        elements[j] = tmp;
    }
}

/*
 * Helper function for Quicksort
 */
static tag_type median3(tag_type elements[], int left, int right)
{
    int center = (left + right) / 2;

    if (elements[left].tag > elements[center].tag)
        swap(&elements[left], &elements[center]);
    if (elements[left].tag > elements[right].tag)
        swap(&elements[left], &elements[right]);
    if (elements[center].tag > elements[right].tag)
        swap(&elements[center], &elements[right]);

    swap(&elements[center], &elements[right - 1]);
    return elements[right - 1];
}

/*
 * Quicksort algorithm
 *
 * The "median of three" pivot selection eliminates
 * the bad case of already sorted input.
 *
 * We use insertion_sort for smaller sub-arrays,
 * because it is faster in this case.
 *
 * For details see: "Data Structures and Algorithm
 * Analysis in C" by Mark Allen Weiss.
 */
static void quicksort(tag_type elements[], int left, int right)
{
    tag_type pivot;
    const int array_size_cutoff = 4;
    if (left + array_size_cutoff <= right) {
        pivot = median3(elements, left, right);

        int i = left;
        int j = right - 1;

        while (true) {
            while (elements[++i].tag < pivot.tag)
                ;
            while (elements[--j].tag > pivot.tag)
                ;

            if (i < j)
                swap(&elements[i], &elements[j]);
            else
                break;
        }

        swap(&elements[i], &elements[right - 1]);

        quicksort(elements, left, i - 1);
        quicksort(elements, i + 1, right);
    } else {
        insertion_sort(elements + left, right - left + 1);
    }
}

/*
 * Frontend for the sorting algorithm
 *
 * Sorts an array of tagged pointers
 * with <number> elements.
 */
void tag_sort(tag_type elements[], int number) { quicksort(elements, 0, number - 1); }
